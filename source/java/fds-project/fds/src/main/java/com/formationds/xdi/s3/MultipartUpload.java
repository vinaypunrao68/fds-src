package com.formationds.xdi.s3;

import com.amazonaws.util.json.JSONArray;
import com.amazonaws.util.json.JSONException;
import com.amazonaws.util.json.JSONObject;
import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.DigestUtil;
import com.formationds.util.async.AsyncSemaphore;
import com.formationds.util.blob.Mode;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.FdsObjectFrame;
import com.formationds.xdi.io.AsyncObjectReader;
import com.formationds.xdi.io.BlobSpecifier;
import com.formationds.xdi.io.ByteBufferInputStream;
import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;
import org.apache.commons.io.IOUtils;
import org.json.JSONWriter;

import java.io.*;
import java.nio.ByteBuffer;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

public class MultipartUpload {
    private BlobSpecifier specifier;
    private AsyncAm am;
    private int fdsObjectSize;
    private String uploadId;
    private final long MAX_PART_DATA_SIZE = 5L * 1024L * 1024L * 1024L;
    private final int PART_HEADER_SIZE = 40;
    private final long MAX_PART_SIZE = PART_HEADER_SIZE + MAX_PART_DATA_SIZE;
    private final long MAX_PARTS = 10000;
    private Executor ioExecutor;
    private final Object executorInitLock = new Object();

    public MultipartUpload(BlobSpecifier specifier, AsyncAm am, int fdsObjectSize, String uploadId) {
        this.specifier = specifier;
        this.am = am;
        this.fdsObjectSize = fdsObjectSize;
        this.uploadId = uploadId;
    }

    public CompletableFuture<Void> uploadPart(InputStream inputStream, int partNumber, long contentLength) {
        if(contentLength > MAX_PART_DATA_SIZE)
            throw new IllegalArgumentException("Content length exceeds maximum content length");

        String name = getTempBlobName();
        ByteBuffer header = ByteBuffer.allocate(PART_HEADER_SIZE);  // empty header as placeholder
        DigestInputStream dis = new DigestInputStream(inputStream, DigestUtil.newMd5());
        InputStream streamWithHeader = new SequenceInputStream(new ByteBufferInputStream(header), dis);

        return am.startBlobTx(specifier.getDomainName(), specifier.getVolumeName(), name, 0)
                .thenCompose(txId -> {
                    CompletableFuture<ByteBuffer> readFuture = CompletableFuture.completedFuture(null);
                    CompletableFuture<ByteBuffer> firstRead = null; // NB: we will delay writing the first frame until we have the etag
                    List<CompletableFuture<Void>> writeFutures = new ArrayList<>();
                    FdsObjectFrame firstFrame = null;

                    // write all frames except for the first
                    for (FdsObjectFrame frame : FdsObjectFrame.frames(getPartOffset(partNumber), contentLength + PART_HEADER_SIZE, fdsObjectSize)) {
                        CompletableFuture<ByteBuffer> objectReadFuture = readFuture.thenCompose(_null -> readFrame(streamWithHeader, frame));
                        readFuture = objectReadFuture;

                        if(firstRead != null) {
                            writeFutures.add(objectReadFuture.thenCompose(buf ->
                                            am.updateBlob(specifier.getDomainName(), specifier.getVolumeName(), name, txId, buf.slice(), fdsObjectSize, new ObjectOffset(frame.objectOffset), false)
                            ));
                        } else {
                            firstFrame = frame;
                            firstRead = objectReadFuture;
                        }
                    }

                    // now write the first frame - with header
                    final CompletableFuture<ByteBuffer> finalFirstFrameRead = firstRead;
                    final FdsObjectFrame finalFirstFrame = firstFrame;
                    CompletableFuture<Void> firstFrameWrite = readFuture
                            .thenCompose(_last -> finalFirstFrameRead)
                            .thenCompose(first -> {
                                ByteBuffer hdr = first.slice();
                                header.limit(PART_HEADER_SIZE);
                                header.putLong(contentLength);
                                header.put(dis.getMessageDigest().digest());
                                return am.updateBlob(specifier.getDomainName(), specifier.getVolumeName(), name, txId, first.slice(), fdsObjectSize, new ObjectOffset(finalFirstFrame.objectOffset), false);
                            });

                    writeFutures.add(firstFrameWrite);

                    CompletableFuture<Void> writeCompleteFuture = CompletableFuture.completedFuture(null);
                    for(CompletableFuture<Void> f : writeFutures)
                        writeCompleteFuture = writeCompleteFuture.thenCompose(_null -> f);

                    return writeCompleteFuture.whenComplete((_null, ex) -> {
                        if(ex != null) am.abortBlobTx(specifier.getDomainName(), specifier.getVolumeName(), name, txId);
                        else am.commitBlobTx(specifier.getDomainName(), specifier.getVolumeName(), name, txId);
                    });
                });
    }

    private CompletionStage<ByteBuffer> readFrame(InputStream inputStream, FdsObjectFrame frame) {
        CompletableFuture<ByteBuffer> readResponse = new CompletableFuture<>();
        getIoExecutor().execute(() -> {
            try {
                byte[] bytes = new byte[fdsObjectSize];
                IOUtils.readFully(inputStream, bytes, frame.internalOffset, frame.internalLength);
                readResponse.complete(ByteBuffer.wrap(bytes));
            } catch(IOException ex) {
                readResponse.completeExceptionally(ex);
            }
        });
        return readResponse;
    }

    private Executor getIoExecutor() {
        if(ioExecutor == null) {
            synchronized (executorInitLock) {
                if(ioExecutor == null) {
                    ioExecutor = Executors.newSingleThreadExecutor();
                }
            }
        }
        return ioExecutor;
    }

    public CompletableFuture<Void> initiate() {
        return am.updateBlobOnce(specifier.getDomainName(), specifier.getVolumeName(), getTempBlobName(), Mode.TRUNCATE.getValue(), ByteBuffer.allocate(0), 0, new ObjectOffset(0), Collections.emptyMap());
    }

    public CompletableFuture<BlobDescriptor> complete() {
        return listParts().thenCompose(parts -> {
            String multipartData = PartInfo.toString(parts);
            String etag = computeEtag(parts);


            Map<String, String> metadata = new HashMap<String, String>();
            metadata.put("etag", etag);
            metadata.put("s3-multipart-index",  multipartData);

            CompletableFuture<Void> updateMetadata = am.startBlobTx(specifier.getDomainName(), specifier.getVolumeName(), getTempBlobName(), 0)
                    .thenCompose(txId ->
                                    am.updateMetadata(specifier.getDomainName(), specifier.getVolumeName(), getTempBlobName(), txId, metadata)
                                            .whenComplete((d, ex) -> {
                                                if (ex != null) {
                                                    am.abortBlobTx(specifier.getDomainName(), specifier.getVolumeName(), getTempBlobName(), txId);
                                                }
                                            })
                                            .thenCompose(d -> am.commitBlobTx(specifier.getDomainName(), specifier.getDomainName(), getTempBlobName(), txId)));

            return updateMetadata.thenCompose(_null -> am.renameBlob(specifier.getDomainName(), specifier.getVolumeName(), getTempBlobName(), specifier.getBlobName()));
        });
    }

    public static AsyncObjectReader read(AsyncAm am, BlobSpecifier specifier, List<PartInfo> parts, int fdsObjectSize) {
        ArrayList<FdsObjectFrame> frames = new ArrayList<>();
        for(PartInfo partInfo : parts) {
            for(FdsObjectFrame frame : partInfo.getFrames(fdsObjectSize)) {
                frames.add(frame);
            }
        }

        return new AsyncObjectReader(am, specifier, frames);
    }

    private String computeEtag(List<PartInfo> parts) {
        MessageDigest md5 = DigestUtil.newMd5();
        for(PartInfo pi : parts)
            md5.update(Hex.encodeHexString(pi.getEtag()).getBytes());

        return Hex.encodeHexString(md5.digest()) + "-" + parts.size();
    }

    public CompletableFuture<Void> cancel() {
        return am.deleteBlob(specifier.getDomainName(), specifier.getVolumeName(), getTempBlobName());
    }

    public CompletableFuture<List<PartInfo>> listParts() {
        AsyncSemaphore limiter = new AsyncSemaphore(20);

        List<CompletableFuture<PartInfo>> partInfoFutures = new ArrayList<>();
        for(int i = 0; i < MAX_PARTS; i++) {
            final int partNumber = i;
            CompletableFuture<PartInfo> partInfoFuture = new CompletableFuture<>();

            limiter.execute(() ->
                    am.getBlob(specifier.getDomainName(), specifier.getVolumeName(), getTempBlobName(), PART_HEADER_SIZE, new ObjectOffset(getPartOffset(partNumber)))
                            .whenComplete((buf, ex) -> {
                                if (ex != null) {
                                    if (ex instanceof ApiException && ((ApiException) ex).getErrorCode() == ErrorCode.MISSING_RESOURCE)
                                        partInfoFuture.complete(null);
                                    else
                                        partInfoFuture.completeExceptionally(ex);
                                }

                                partInfoFuture.complete(decodeHeader(buf, partNumber));
                            }));

            partInfoFutures.add(partInfoFuture);
        }

        CompletableFuture<List<PartInfo>> result = CompletableFuture.completedFuture(new ArrayList<>());
        for(CompletableFuture<PartInfo> partFuture : partInfoFutures) {
            result = result.thenCombine(partFuture, (lst, partInfo) -> {
               if(partInfo != null)
                   lst.add(partInfo);
                return lst;
            });
        }
        return result;
    }

    private PartInfo decodeHeader(ByteBuffer buf, int partNumber) {
        long length = buf.getLong();
        byte[] etag = new byte[32];
        buf.get(etag);

        return new PartInfo(partNumber, etag, length, getPartOffset(partNumber), PART_HEADER_SIZE);
    }

    private long partObjectLength() {
        long length = MAX_PART_SIZE / fdsObjectSize;
        if(MAX_PART_SIZE % fdsObjectSize != 0)
            length++;
        return length;
    }

    private long getPartOffset(int partNumber) {
        return partObjectLength() * (partNumber - 1);
    }

    private String getTempBlobName() {
        return S3Namespace.fds().blobName("/uploads/" + specifier.getBlobName() + "-" + uploadId);
    }

    public static class PartInfo {
        private int partNumber;
        private byte[] etag;
        private long length;
        private long startObjectOffset;

        public int getStartObjectByteOffset() {
            return startObjectByteOffset;
        }

        public long getStartObjectOffset() {
            return startObjectOffset;
        }

        private int startObjectByteOffset;

        public int getPartNumber() {
            return partNumber;
        }

        public byte[] getEtag() {
            return etag;
        }

        public long getLength() {
            return length;
        }

        public PartInfo(int partNumber, byte[] etag, long length, long objectOffset, int byteOffset) {
            this.partNumber = partNumber;
            this.etag = etag;
            this.length = length;
            startObjectByteOffset = byteOffset;
            startObjectOffset = objectOffset;
        }

        public Iterable<FdsObjectFrame> getFrames(int objectSize) {
            return FdsObjectFrame.frames(objectSize * startObjectOffset + startObjectByteOffset, length, objectSize);
        }

        public static String toString(Collection<PartInfo> partInfos) {
            JSONObject object = new JSONObject();
            JSONArray parts = new JSONArray();

            try {
                object.put("version", "1");

                for(PartInfo info : partInfos) {
                    JSONObject partObject = new JSONObject();
                    partObject.put("partNumber", info.getPartNumber());
                    partObject.put("etag", Hex.encodeHexString(info.getEtag()));
                    partObject.put("length", info.getLength());
                    partObject.put("dataObjectOffset", info.getStartObjectOffset());
                    partObject.put("dataByteOffset", info.getStartObjectByteOffset());
                    parts.put(partObject);
                }

                object.put("parts", parts);
                return object.toString();

            } catch (JSONException e) {
                throw new AssertionError("could not make JSON for multipart part list", e);
            }
        }

        public static List<PartInfo> parse(String input) {
            try {
                JSONObject object = new JSONObject(input);
                String version = object.getString("version");
                if(!version.equals("1"))
                    throw new IllegalArgumentException("unrecognized multipart version");

                ArrayList<PartInfo> partInfos = new ArrayList<>();
                JSONArray parts = object.getJSONArray("parts");
                for(int i = 0; i < parts.length(); i++) {
                    JSONObject part = parts.getJSONObject(i);

                    int partNumber = part.getInt("partNumber");
                    byte[] etag = Hex.decodeHex(part.getString("etag").toCharArray());
                    long length =  part.getLong("length");
                    long objectOffset = part.getLong("dataObjectOffset");
                    int byteOffset = part.getInt("dataByteOffset");

                    partInfos.add(new PartInfo(partNumber, etag, length, objectOffset, byteOffset));
                }

                return partInfos;
            } catch (JSONException | DecoderException e) {
                throw new AssertionError("could not parse multipart JSON", e);
            }
        }

        public static long computeLength(List<PartInfo> parts) {
            return parts.stream().collect(Collectors.summingLong(p -> p.getLength()));
        }
    }
}
