package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

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
        AmService.Iface am = new AmService.Client(new TBinaryProtocol(socket));
        TSocket omTransport = new TSocket("localhost", 9090);
        omTransport.open();
        ConfigurationService.Iface config = new ConfigurationService.Client(new TBinaryProtocol(omTransport));

        System.out.println("Creating volume 'Volume1', policy: 4kb blocksize");
        try {
            config.createVolume(DOMAIN_NAME, VOLUME_NAME, new VolumePolicy(4 * 1024));
            config.createVolume(DOMAIN_NAME, "Volume2", new VolumePolicy(4 * 1024));
        } catch(ApiException e) {
            e.printStackTrace();
        }
        Thread.sleep(4000);
              
        System.out.println("Creating volume 'Volume1', policy: 2MB blocksize");
        VolumePolicy volumePolicy = new VolumePolicy(2 * 1024 * 1024);
        config.createVolume(DOMAIN_NAME, VOLUME_NAME, volumePolicy);
        Thread.sleep(4000);

        List<VolumeDescriptor> volumeDescriptors = config.listVolumes(DOMAIN_NAME);
        System.out.println("Found " + volumeDescriptors.size() + " volumes");

        int length = 2 * 1024 * 1024; // volumePolicy.getMaxObjectSizeInBytes();
        byte[] putData = new byte[length];
        byte pattern = (byte)255;
        Arrays.fill(putData, pattern);

        System.out.println("Creating object '"+BLOB_NAME+"', size: " + length + " bytes");
        int offCount = 0;
        for (offCount = 0; offCount < 10; offCount++) {
            am.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                              ByteBuffer.wrap(putData), length,
                              new ObjectOffset(offCount), ByteBuffer.wrap(new byte[0]), false);
        }
        am.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                          ByteBuffer.wrap(putData), 0, new ObjectOffset(offCount), ByteBuffer.wrap(new byte[0]), true);

        ByteBuffer data = am.getBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                                         length, new ObjectOffset(0));

        // StringBuilder sb = new StringBuilder();
        // for (byte b : data.array()) {
        // sb.append(String.format("%02x", b));
        // }
        // System.out.println("Done getting object " + sb.toString());

        // System.out.println("Writing arbitrary length stream, size: a few bytes");
        // InputStream inputStream = IOUtils.toInputStream("hello, world!");
        // new StreamWriter(maxObjSize, client).write(DOMAIN_NAME, VOLUME_NAME, "stream.bin", inputStream, new HashMap<>());
        
        System.out.println("stating blob");
        System.out.println(am.statBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME));
        
        System.out.println("stating volume");
        System.out.println(config.statVolume(DOMAIN_NAME, VOLUME_NAME));

        System.out.println("deleting blob");
        am.deleteBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);

        System.out.println("volume list");
        System.out.println(config.listVolumes(DOMAIN_NAME));

        System.out.println("deleting volume");
        config.deleteVolume(DOMAIN_NAME, VOLUME_NAME);
        System.out.println("All done.");
    }
}
