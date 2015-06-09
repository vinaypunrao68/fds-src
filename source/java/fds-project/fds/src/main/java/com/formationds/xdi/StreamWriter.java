package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.blob.Mode;
import com.formationds.apis.XdiService;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import org.apache.commons.codec.binary.Hex;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.Map;

public class StreamWriter {
    private int objectSize;
    private XdiService.Iface am;

    private static ThreadLocal<byte[]> localBytes = new ThreadLocal<>();

    public StreamWriter(int objectSize, XdiService.Iface am) {
        this.objectSize = objectSize;
        this.am = am;
    }

    public byte[] write(String domainName, String volumeName, String blobName, InputStream input, Map<String, String> metadata) throws Exception {
        long objectOffset = 0;
        MessageDigest md = MessageDigest.getInstance("MD5");
        byte[] digest = new byte[0];

        TxDescriptor tx = null;
        try {
            if (localBytes.get() == null || localBytes.get().length != objectSize) {
                localBytes.set(new byte[objectSize]);
            }

            byte[] buf = localBytes.get();

            int firstReadCount = readFully(input, buf);
            int nextByte = 0;
            if(firstReadCount != -1) {
                nextByte = input.read();
                md.update(buf, 0, firstReadCount);
            }

            if(firstReadCount == -1 || nextByte == -1) {
                // single object case
                digest = md.digest();
                metadata.put("etag", Hex.encodeHexString(digest));
                am.updateBlobOnce(domainName, volumeName, blobName,
                                  Mode.TRUNCATE.getValue(), ByteBuffer.wrap(buf, 0, Math.max(0, firstReadCount)),
                                  Math.max(0, firstReadCount), new ObjectOffset(0), metadata);
                return digest;
            } else {
                // multi object case
                tx = am.startBlobTx(domainName, volumeName, blobName, Mode.TRUNCATE.getValue());

                // push first read to FDS
                am.updateBlob(domainName, volumeName, blobName, tx,
                        ByteBuffer.wrap(buf, 0, firstReadCount), firstReadCount,
                        new ObjectOffset(objectOffset));
                objectOffset++;

                // reassemble second read and push to FDS
                int secondReadCount = Math.max(readFully(input, buf, 1), 0) + 1;
                buf[0] = (byte) nextByte;
                md.update(buf, 0, secondReadCount);
                am.updateBlob(domainName, volumeName, blobName, tx,
                        ByteBuffer.wrap(buf, 0, secondReadCount), secondReadCount,
                        new ObjectOffset(objectOffset));
                objectOffset++;

                // read remaining
                for (int read = readFully(input, buf); read != -1; read = readFully(input, buf)) {
                    md.update(buf, 0, read);
                    am.updateBlob(domainName, volumeName, blobName, tx,
                            ByteBuffer.wrap(buf, 0, read), read,
                            new ObjectOffset(objectOffset));
                    objectOffset++;
                }
            }

            digest = md.digest();
            metadata.put("etag", Hex.encodeHexString(digest));

            am.updateMetadata(domainName, volumeName, blobName, tx, metadata);
            if(tx != null)
                am.commitBlobTx(domainName, volumeName, blobName, tx);
            return digest;
        } catch (Exception e) {
            if(tx != null)
                am.abortBlobTx(domainName, volumeName, blobName, tx);
            throw e;
        }
    }

    public int readFully(InputStream inputStream, byte[] buf) throws IOException {
        return readFully(inputStream, buf, 0);
    }

    public int readFully(InputStream inputStream, byte[] buf, int initialOffset) throws IOException {
        int bytesRead = 0;
        int fillLength = buf.length - initialOffset;
        while (bytesRead < fillLength) {
            int read = inputStream.read(buf, bytesRead + initialOffset, fillLength - bytesRead);
            if (read == -1) break;
            bytesRead += read;
        }
        return bytesRead == 0 ? -1 : bytesRead;
    }

}
