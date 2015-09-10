package com.formationds.xdi.s3;

import com.amazonaws.util.json.JSONArray;
import com.amazonaws.util.json.JSONException;
import com.amazonaws.util.json.JSONObject;
import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.*;
import com.formationds.util.DigestUtil;
import com.formationds.util.blob.Mode;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.FdsObjectFrame;
import com.formationds.xdi.io.AsyncObjectReader;
import com.formationds.xdi.io.BlobSpecifier;
import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;

import java.io.*;
import java.nio.ByteBuffer;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

public class MultipartUpload {
    private BlobSpecifier specifier;
    private AsyncAm am;
    private int fdsObjectSize;
    private String uploadId;
    private final long MAX_PART_DATA_SIZE = 5L * 1024L * 1024L * 1024L;
    private final long MAX_PARTS = 10000;
    private Executor ioExecutor;
    private final Object executorInitLock = new Object();

    public MultipartUpload(BlobSpecifier specifier, AsyncAm am, int fdsObjectSize, String uploadId) {
        this.specifier = specifier;
        this.am = am;
        this.fdsObjectSize = fdsObjectSize;
        this.uploadId = uploadId;
    }

    public CompletableFuture<byte[]> uploadPart(InputStream inputStream, int partNumber, long contentLength) {
        if(partNumber < 1 || partNumber > MAX_PARTS)
            throw new IllegalArgumentException("Part number must be between 1-" + MAX_PARTS + " (inclusive)");

        if(contentLength > MAX_PART_DATA_SIZE) {
            throw new IllegalArgumentException("Content length exceeds maximum content length for upload part");
        }

        String dataBlobName = getDataBlobName();
        String partMetadataBlobName = getPartMetadataBlobName(partNumber);

        long offset = getPartOffset(partNumber);

        DigestInputStream dis = new DigestInputStream(inputStream, DigestUtil.newMd5());

        return am.startBlobTx(specifier.getDomainName(), specifier.getVolumeName(), dataBlobName, 0).thenCompose(txId -> {
            CompletableFuture<ByteBuffer> readCompleteFuture = CompletableFuture.completedFuture(null);
            CompletableFuture<Void> writeCompleteFuture = CompletableFuture.completedFuture(null);

            for(FdsObjectFrame frame : FdsObjectFrame.frames(offset * fdsObjectSize, contentLength, fdsObjectSize)) {
               CompletableFuture<ByteBuffer> readFuture = readCompleteFuture.thenCompose(c -> readFrame(dis, frame));
               readCompleteFuture = readFuture;

               CompletableFuture<Void> writeFuture = readFuture.thenCompose(buf ->
                               am.updateBlob(specifier.getDomainName(), specifier.getVolumeName(), dataBlobName, txId, buf, buf.remaining(), new ObjectOffset(frame.objectOffset), false)
               );

               writeCompleteFuture = writeCompleteFuture.thenCompose(_null -> writeFuture);
            }

            writeCompleteFuture = writeCompleteFuture.whenComplete((_null, ex) -> {
                if (ex != null)
                    am.abortBlobTx(specifier.getDomainName(), specifier.getVolumeName(), dataBlobName, txId);
                else
                    am.commitBlobTx(specifier.getDomainName(), specifier.getVolumeName(), dataBlobName, txId);
            });


            CompletableFuture<byte[]> digest = readCompleteFuture.thenApply(_buf -> dis.getMessageDigest().digest());

            CompletableFuture<Void> metadataBlobWriteFuture =
                writeCompleteFuture.thenCompose(_null ->  digest).thenCompose(hash -> {
                    Map<String, String> metadata = new HashMap<>();
                    metadata.put("length", Long.toString(contentLength));
                    metadata.put("etag", Hex.encodeHexString(hash));

                    return am.updateBlobOnce(specifier.getDomainName(), specifier.getVolumeName(), partMetadataBlobName, Mode.TRUNCATE.getValue(), ByteBuffer.allocate(0), 0, new ObjectOffset(0), metadata);
                });

            return metadataBlobWriteFuture
                    .thenCompose(_null -> digest);
        });
    }

//    public CompletableFuture<byte[]> uploadPart(InputStream inputStream, int partNumber, long contentLength) {
//        if(contentLength > MAX_PART_DATA_SIZE)
//            throw new IllegalArgumentException("Content length exceeds maximum content length");
//
//        String name = getDataBlobName();
//        DigestInputStream dis = new DigestInputStream(inputStream, DigestUtil.newMd5());
//        InputStream streamWithHeader = new SequenceInputStream(new ByteBufferInputStream(ByteBuffer.allocate(PART_HEADER_SIZE)), dis);
//
//        return am.startBlobTx(specifier.getDomainName(), specifier.getVolumeName(), name, 0)
//                .thenCompose(txId -> {
//                    CompletableFuture<ByteBuffer> readFuture = CompletableFuture.completedFuture(null);
//                    CompletableFuture<ByteBuffer> firstRead = null; // NB: we will delay writing the first frame until we have the etag
//                    List<CompletableFuture<Void>> writeFutures = new ArrayList<>();
//                    FdsObjectFrame firstFrame = null;
//
//                    // write all frames except for the first
//                    for (FdsObjectFrame frame : FdsObjectFrame.frames(getPartOffset(partNumber) * partObjectLength(), contentLength + PART_HEADER_SIZE, fdsObjectSize)) {
//                        CompletableFuture<ByteBuffer> objectReadFuture = readFuture.thenCompose(_null -> readFrame(streamWithHeader, frame));
//                        readFuture = objectReadFuture;
//
//                        if(firstRead != null) {
//                            writeFutures.add(objectReadFuture.thenCompose(buf ->
//                                            am.updateBlob(specifier.getDomainName(), specifier.getVolumeName(), name, txId, buf.slice(), fdsObjectSize, new ObjectOffset(frame.objectOffset), false)
//                            ));
//                        } else {
//                            firstFrame = frame;
//                            firstRead = objectReadFuture;
//                        }
//                    }
//
//                    CompletableFuture<byte[]> digest = readFuture.thenApply(_null -> dis.getMessageDigest().digest());
//
//                    // now write the first frame - with header
//                    final CompletableFuture<ByteBuffer> finalFirstFrameRead = firstRead;
//                    final FdsObjectFrame finalFirstFrame = firstFrame;
//                    CompletableFuture<Void> firstFrameWrite = readFuture
//                            .thenCompose(_last -> finalFirstFrameRead)
//                            .thenCompose(first ->
//                                    digest.thenCompose(d -> {
//                                        ByteBuffer hdr = first.slice();
//                                        hdr.limit(PART_HEADER_SIZE);
//                                        hdr.putLong(contentLength);
//                                        hdr.put(dis.getMessageDigest().digest());
//
//                                        return am.updateBlob(specifier.getDomainName(), specifier.getVolumeName(), name, txId, first.slice(), fdsObjectSize, new ObjectOffset(finalFirstFrame.objectOffset), false);
//                                }));
//
//                    writeFutures.add(firstFrameWrite);
//
//                    CompletableFuture<Void> writeCompleteFuture = CompletableFuture.completedFuture(null);
//                    for(CompletableFuture<Void> f : writeFutures)
//                        writeCompleteFuture = writeCompleteFuture.thenCompose(_null -> f);
//
//                    return writeCompleteFuture.whenComplete((_null, ex) -> {
//                        if (ex != null)
//                            am.abortBlobTx(specifier.getDomainName(), specifier.getVolumeName(), name, txId);
//                        else
//                            am.commitBlobTx(specifier.getDomainName(), specifier.getVolumeName(), name, txId);
//                    }).thenCompose(_null -> digest);
//                });
//    }

    private CompletionStage<ByteBuffer> readFrame(InputStream inputStream, FdsObjectFrame frame) {
        CompletableFuture<ByteBuffer> readResponse = new CompletableFuture<>();
        getIoExecutor().execute(() -> {
            try {
                byte[] bytes = new byte[fdsObjectSize];

                readFully(inputStream, bytes, frame.internalOffset, frame.internalLength);
                readResponse.complete(ByteBuffer.wrap(bytes));
            } catch(Exception ex) {
                readResponse.completeExceptionally(ex);
            }
        });
        return readResponse;
    }

    private int readFully(InputStream inputStream, byte[] bytes, int internalOffset, int internalLength) throws IOException {
        int read = 0;
        while(true) {
            if(read == internalLength)
                return read;

            int r = inputStream.read(bytes, internalOffset + read, internalLength - read);
            if(r == -1)
                return read > 0 ? read : -1;
        }
    }

    private Executor getIoExecutor() {
        if(ioExecutor == null) {
            synchronized (executorInitLock) {
                if(ioExecutor == null) {
                    ioExecutor = Executors.newCachedThreadPool();
                }
            }
        }
        return ioExecutor;
    }

    public CompletableFuture<Void> initiate() {
        ByteBuffer b = ByteBuffer.allocate(fdsObjectSize);
        return am.updateBlobOnce(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), Mode.TRUNCATE.getValue(), b, fdsObjectSize, new ObjectOffset(0), Collections.emptyMap());
    }

    public CompletableFuture<BlobDescriptor> complete() {
        return listParts().thenCompose(parts -> {
            String multipartData = PartInfo.toString(parts);
            String etag = computeEtag(parts);


            Map<String, String> metadata = new HashMap<String, String>();
            metadata.put("etag", etag);
            metadata.put("s3-multipart-index",  multipartData);

            CompletableFuture<Void> updateMetadata = am.startBlobTx(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), 0)
                    .thenCompose(txId ->
                                    am.updateMetadata(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), txId, metadata)
                                            .whenComplete((d, ex) -> {
                                                if (ex != null) {
                                                    am.abortBlobTx(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), txId);
                                                }
                                            })
                                            .thenCompose(d -> am.commitBlobTx(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), txId)));

            return updateMetadata.thenCompose(_null ->
                    am.renameBlob(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), specifier.getBlobName())
            );
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
        return am.deleteBlob(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName());
    }

//    public CompletableFuture<List<PartInfo>> listParts() {
//        AsyncSemaphore limiter = new AsyncSemaphore(20);
//
//        List<CompletableFuture<PartInfo>> partInfoFutures = new ArrayList<>();
//        for(int i = 1; i <= MAX_PARTS; i++) {
//            final int partNumber = i;
//            CompletableFuture<PartInfo> partInfoFuture = new CompletableFuture<>();
//
//            limiter.execute(() ->
//                    am.getBlob(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), PART_HEADER_SIZE, new ObjectOffset(getPartOffset(partNumber)))
//                            .whenComplete((buf, ex) -> {
//                                int j = partNumber;
//                                if (ex != null) {
//                                    if (ex instanceof ApiException && ((ApiException) ex).getErrorCode() == ErrorCode.MISSING_RESOURCE)
//                                        partInfoFuture.complete(null);
//                                    else
//                                        partInfoFuture.completeExceptionally(ex);
//                                }
//                                else {
//                                    partInfoFuture.complete(decodeHeader(buf, partNumber));
//                                }
//                            }));
//
//            partInfoFutures.add(partInfoFuture);
//        }
//
//        CompletableFuture<List<PartInfo>> result = CompletableFuture.completedFuture(new ArrayList<>());
//        for(CompletableFuture<PartInfo> partFuture : partInfoFutures) {
//            result = result.thenCombine(partFuture, (lst, partInfo) -> {
//               if(partInfo != null)
//                   lst.add(partInfo);
//                return lst;
//            });
//        }
//        return result;
//    }

    private CompletableFuture<List<PartInfo>> listParts() {
        Pattern pattern = Pattern.compile(".*part-([0-9]+)$");
        return am.volumeContents(specifier.getDomainName(), specifier.getVolumeName(), 20000, 0, uploadBlobName(""), PatternSemantics.PREFIX, BlobListOrder.UNSPECIFIED, false)
                .thenApply(contents -> {
                    ArrayList<PartInfo> parts = new ArrayList<>();

                    for (BlobDescriptor desc : contents) {
                        Matcher matcher = pattern.matcher(desc.getName());
                        if (matcher.matches()) {
                            try {
                                int partNumber = Integer.parseInt(matcher.group(1));
                                PartInfo partInfo = getPartInfo(desc, partNumber);
                                parts.add(partInfo);
                            } catch(Exception ex) {
                                // continue without part
                            }
                        }
                    }

                    return parts;
                });
    }

    private PartInfo getPartInfo(BlobDescriptor desc, int partNumber) throws DecoderException {
        String etag = desc.getMetadata().get("etag");
        long length = Integer.parseInt(desc.getMetadata().get("length"));
        return new PartInfo(partNumber, Hex.decodeHex(etag.toCharArray()), length, getPartOffset(partNumber), 0);
    }

    private long partObjectLength() {
        long length = MAX_PART_DATA_SIZE / fdsObjectSize;
        if(MAX_PART_DATA_SIZE % fdsObjectSize != 0)
            length++;
        return length;
    }

    private long getPartOffset(int partNumber) {
        return 1 + partObjectLength() * (partNumber - 1);
    }

    private String uploadBlobName(String arg) {
        return S3Namespace.fds().blobName("/uploads/" + specifier.getBlobName() + "-" + uploadId + "/" + arg);
    }

    private String getDataBlobName() {
        return uploadBlobName("data");
    }

    private String getPartMetadataBlobName(int partNumber) {
        return uploadBlobName("part-" + partNumber);
    }

    public static List<PartInfo> getPartInfoList(BlobDescriptor stat) {
        String partInfo = stat.getMetadata().getOrDefault("s3-multipart-index", null);
        if(partInfo == null)
            return null;
        return PartInfo.parse(partInfo);
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
