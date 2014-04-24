package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.ObjectOffset;
import com.formationds.xdi.shim.VolumePolicy;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class SmokeTest {

    public static final String DOMAIN_NAME = "FDS";
    public static final String VOLUME_NAME = "Volume1";
    public static final String BLOB_NAME = "someBytes.bin";

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            System.out.println("Usage: SmokeTest host port");
            return;
        }

        TSocket socket = new TSocket(args[0], Integer.parseInt(args[1]));
        socket.open();
        AmShim.Iface client = new AmShim.Client(new TBinaryProtocol(socket));

        System.out.println("Creating volume 'Volume1', policy: 2MB blocksize");
        VolumePolicy volumePolicy = new VolumePolicy(2 * 1024 * 1024);
        client.createVolume(DOMAIN_NAME, VOLUME_NAME, volumePolicy);
        Thread.sleep(4000);

        System.out.println("Creating object 'someBytes.bin', size: 8192 bytes");
        int length = volumePolicy.getMaxObjectSizeInBytes();
        byte[] putData = new byte[length];
        byte pattern = (byte)255;
        Arrays.fill(putData, pattern);
        for (offCount = 0; offCount < 10; offCount++) {
            client.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                              ByteBuffer.wrap(putData), length,
                              new ObjectOffset(i), ByteBuffer.wrap(new byte[0]), false);
        }

        i--;
        client.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                          ByteBuffer.wrap(putData), 0, new ObjectOffset(i), ByteBuffer.wrap(new byte[0]), true);

        ByteBuffer data = client.getBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, length, 0);

        // StringBuilder sb = new StringBuilder();
        // for (byte b : data.array()) {
        // sb.append(String.format("%02x", b));
        // }
        // System.out.println("Done getting object " + sb.toString());

        // System.out.println("Writing arbitrary length stream, size: a few bytes");
        // InputStream inputStream = IOUtils.toInputStream("hello, world!");
        // new StreamWriter(maxObjSize, client).write(DOMAIN_NAME, VOLUME_NAME, "stream.bin", inputStream, new HashMap<>());

        // System.out.println("Deleting object 'someBytes.bin'");
        // client.deleteBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);
        // System.out.println("Deleting volume 'Volume1'");
        client.deleteVolume(DOMAIN_NAME, VOLUME_NAME);
        System.out.println("All done.");
    }
}
