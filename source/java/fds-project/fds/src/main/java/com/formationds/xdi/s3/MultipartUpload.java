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
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;

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
    public static final String S3_MULTIPART_INDEX_METADATA_KEY = "s3-multipart-index";
    public static final String LENGTH_METADATA_KEY = "length";
    public static final String ETAG_METADATA_KEY = "etag";
    public static final String LAST_MODIFIED_METADATA_KEY = "lastModified";
    private BlobSpecifier specifier;
    private Optional<BlobDescriptor> existingBlobDescriptor;
    private AsyncAm am;
    private int fdsObjectSize;
    private String uploadId;
    private final long MAX_PART_DATA_SIZE = 5L * 1024L * 1024L * 1024L;
    private final long MAX_PARTS = 10000;
    private Executor ioExecutor;
    private final Object executorInitLock = new Object();

    public MultipartUpload(BlobSpecifier specifier, Optional<BlobDescriptor> existingBlobDescriptor, AsyncAm am, int fdsObjectSize, String uploadId) {
        this.specifier = specifier;
        this.existingBlobDescriptor = existingBlobDescriptor;
        this.am = am;
        this.fdsObjectSize = fdsObjectSize;
        this.uploadId = uploadId;
    }

    public CompletableFuture<byte[]> uploadPart(InputStream inputStream, int partNumber, long contentLength, DateTime now) {
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
                    metadata.put(LENGTH_METADATA_KEY, Long.toString(contentLength));
                    metadata.put(ETAG_METADATA_KEY, Hex.encodeHexString(hash));
                    metadata.put(LAST_MODIFIED_METADATA_KEY, Long.toString(now.getMillis()));

                    return am.updateBlobOnce(specifier.getDomainName(), specifier.getVolumeName(), partMetadataBlobName, Mode.TRUNCATE.getValue(), ByteBuffer.allocate(0), 0, new ObjectOffset(0), metadata);
                });

            return metadataBlobWriteFuture
                    .thenCompose(_null -> digest);
        });
    }

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

            read += r;
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

    public CompletableFuture<Void> initiate(Map<String, String> userMetadata) {
        ByteBuffer b = ByteBuffer.allocate(fdsObjectSize);
        return am.updateBlobOnce(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), Mode.TRUNCATE.getValue(), b, fdsObjectSize, new ObjectOffset(0), userMetadata);
    }

    // TODO: we need a way to transactionally read the metadata to ensure that in the case of an update, we are updating what we think we are
    public CompletableFuture<BlobDescriptor> complete() {
        return listParts().thenCompose(parts -> {
            String multipartData = PartInfo.toString(parts);
            String etag = computeEtag(parts);

            CompletableFuture<Map<String, String>> extantMetadata = new CompletableFuture<Map<String, String>>();


            am.statBlob(specifier.getDomainName(), specifier.getVolumeName(), specifier.getBlobName())
            .whenComplete((bd, ex) -> {
                if (bd != null)
                    extantMetadata.complete(bd.getMetadata());
                else if (ex instanceof ApiException && ((ApiException) ex).getErrorCode() == ErrorCode.MISSING_RESOURCE)
                    extantMetadata.complete(Collections.emptyMap());

                extantMetadata.completeExceptionally(ex);
            });

            return extantMetadata.thenCompose(md -> {

                Map<String, String> metadata = new HashMap<String, String>(md);
                S3MetadataUtility.updateMetadata(metadata, existingBlobDescriptor);
                metadata.put(ETAG_METADATA_KEY, etag);
                metadata.put(S3_MULTIPART_INDEX_METADATA_KEY, multipartData);

                final Map<String, String> metadataFinal = metadata;
                CompletableFuture<Void> updateMetadata = am.startBlobTx(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), 0)
                        .thenCompose(txId ->
                                am.updateMetadata(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName(), txId, metadataFinal)
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
        return listParts().thenCompose(parts -> {
            CompletableFuture<Void> cf = new CompletableFuture<Void>();

            for(PartInfo partInfo : parts) {
                CompletableFuture<Void> deleteFuture = am.deleteBlob(specifier.getDomainName(), specifier.getVolumeName(), getPartMetadataBlobName(partInfo.partNumber));
                cf = cf.thenCompose(_void -> deleteFuture);
            }
            CompletableFuture<Void> deleteDataBlobFuture = am.deleteBlob(specifier.getDomainName(), specifier.getVolumeName(), getDataBlobName());
            cf = cf.thenCompose(_void -> deleteDataBlobFuture);

            return cf;
        });
    }

    public CompletableFuture<List<PartInfo>> listParts() {
        Pattern pattern = Pattern.compile(".*part-([0-9]+)$");
        return am.volumeContents(specifier.getDomainName(), specifier.getVolumeName(), 20000, 0, uploadBlobName(""), PatternSemantics.PCRE, null, BlobListOrder.UNSPECIFIED, false)
                .thenApply(contents -> {
                    ArrayList<PartInfo> parts = new ArrayList<>();

                    for (BlobDescriptor desc : contents.getBlobs()) {
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
        String etag = desc.getMetadata().get(ETAG_METADATA_KEY);
        long length = Integer.parseInt(desc.getMetadata().get(LENGTH_METADATA_KEY));
        long lastModified = Long.parseLong(desc.getMetadata().get(LAST_MODIFIED_METADATA_KEY));
        return new PartInfo(partNumber, Hex.decodeHex(etag.toCharArray()), length, getPartOffset(partNumber), new DateTime(lastModified, DateTimeZone.UTC));
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
        String partInfo = stat.getMetadata().getOrDefault(S3_MULTIPART_INDEX_METADATA_KEY, null);
        if(partInfo == null)
            return null;
        return PartInfo.parse(partInfo);
    }

    public static boolean isMultipartBlob(BlobDescriptor blobDescriptor) {
        return getPartInfoList(blobDescriptor) != null;
    }

    public static class PartInfo {
        private int partNumber;
        private byte[] etag;
        private long length;
        private long startObjectOffset;
        private DateTime lastModified;

        public long getStartObjectOffset() {
            return startObjectOffset;
        }

        public int getPartNumber() {
            return partNumber;
        }

        public byte[] getEtag() {
            return etag;
        }

        public long getLength() {
            return length;
        }

        public PartInfo(int partNumber, byte[] etag, long length, long objectOffset, DateTime lastModified) {
            this.partNumber = partNumber;
            this.etag = etag;
            this.length = length;
            startObjectOffset = objectOffset;
            this.lastModified = lastModified;
        }

        public Iterable<FdsObjectFrame> getFrames(int objectSize) {
            return FdsObjectFrame.frames(objectSize * startObjectOffset, length, objectSize);
        }

        public static String toString(Collection<PartInfo> partInfos) {
            JSONObject object = new JSONObject();
            JSONArray parts = new JSONArray();

            try {
                object.put("version", "1");

                for (PartInfo info : partInfos) {
                    JSONObject partObject = new JSONObject();
                    partObject.put("partNumber", info.getPartNumber());
                    partObject.put("etag", Hex.encodeHexString(info.getEtag()));
                    partObject.put("length", info.getLength());
                    partObject.put("dataObjectOffset", info.getStartObjectOffset());
                    partObject.put("lastModified", info.getLastModified().getMillis());
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
                    DateTime lastModified = new DateTime(part.getLong("lastModified"), DateTimeZone.UTC);

                    partInfos.add(new PartInfo(partNumber, etag, length, objectOffset, lastModified));
                }

                return partInfos;
            } catch (JSONException | DecoderException e) {
                throw new AssertionError("could not parse multipart JSON", e);
            }
        }


        public static long computeLength(List<PartInfo> parts) {
            return parts.stream().collect(Collectors.summingLong(p -> p.getLength()));
        }

        public DateTime lastModified() {
            return lastModified;
        }

        public DateTime getLastModified() {
            return lastModified;
        }
    }
}
