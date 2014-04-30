package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.VolumePolicy;
import org.apache.commons.io.IOUtils;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import com.formationds.xdi.shim.*;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;

public class SmokeTest {

    public static final String DOMAIN_NAME = "FDS";
    public static final String VOLUME_NAME = "Volume1";
    public static final String BLOB_NAME   = "someBytes.bin";

    public static void main(String[] args) throws Exception {
        String host="localhost";
        int port = 9988;
        if (args.length == 2) {
            host = args[0];
            port = Integer.parseInt(args[1]); 
        } else if (args.length > 0) {
            System.out.println("Usage: SmokeTest host port");
            return;
        }

        TSocket socket = new TSocket(host, port);
        socket.open();
        AmShim.Iface client = new AmShim.Client(new TBinaryProtocol(socket));
        System.out.println("Creating volume 'Volume1', policy: 4kb blocksize");
        try {
            client.createVolume(DOMAIN_NAME, VOLUME_NAME, new VolumePolicy(4 * 1024));
        } catch(XdiException e) {
            e.printStackTrace();
        }
        Thread.sleep(4000);

        int maxObjSize = 2 * 1024 * 1024;

        System.out.println("Creating object 'someBytes.bin', size: 8192 bytes");
        int offCount = 0;
        for (offCount = 0; offCount < 10; offCount++) {
            client.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                              ByteBuffer.wrap(new byte[maxObjSize]), maxObjSize,
                              offCount * maxObjSize, false);
        }
        client.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                          ByteBuffer.wrap(new byte[maxObjSize], 0, 0), 0, 0, true);

        // System.out.println("Writing arbitrary length stream, size: a few bytes");
        // InputStream inputStream = IOUtils.toInputStream("hello, world!");
        // new StreamWriter(maxObjSize, client).write(DOMAIN_NAME, VOLUME_NAME, "stream.bin", inputStream, new HashMap<>());

        // System.out.println("Deleting object 'someBytes.bin'");
        // client.deleteBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);
        // System.out.println("Deleting volume 'Volume1'");
        
        System.out.println("stating blob");
        System.out.println(client.statBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME));
        
        System.out.println("stating volume");
        System.out.println(client.statVolume(DOMAIN_NAME, VOLUME_NAME));

        System.out.println("deleting blob");
        client.deleteBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);

        System.out.println("deleting volume");
        client.deleteVolume(DOMAIN_NAME, VOLUME_NAME);
        System.out.println("All done.");
    }
}
