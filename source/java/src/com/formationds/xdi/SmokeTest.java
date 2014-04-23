package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.VolumePolicy;
import org.apache.commons.io.IOUtils;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;

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
        System.out.println("Creating volume 'Volume1', policy: 4kb blocksize");
        client.createVolume(DOMAIN_NAME, VOLUME_NAME, new VolumePolicy(4 * 1024));
        Thread.sleep(4000);

        int maxObjSize = 2 * 1024 * 1024;

        System.out.println("Creating object 'someBytes.bin', size: 8192 bytes");
        client.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, ByteBuffer.wrap(new byte[maxObjSize]), maxObjSize, 0l, false);
        client.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, ByteBuffer.wrap(new byte[maxObjSize]), maxObjSize, maxObjSize, true);

        System.out.println("Writing arbitrary length stream, size: a few bytes");
        InputStream inputStream = IOUtils.toInputStream("hello, world!");
        new StreamWriter(16, client).write(DOMAIN_NAME, VOLUME_NAME, "stream.bin", inputStream, new HashMap<>());

        // System.out.println("Deleting object 'someBytes.bin'");
        // client.deleteBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);
        // System.out.println("Deleting volume 'Volume1'");
        client.deleteVolume(DOMAIN_NAME, VOLUME_NAME);
        System.out.println("All done.");
    }
}
