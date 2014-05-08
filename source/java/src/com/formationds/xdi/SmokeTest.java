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

        System.out.println("Creating volume 'S3Vol', policy: 4kb blocksize");
        try {
            config.createVolume(DOMAIN_NAME, "S3Vol",
                                new VolumePolicy(4 * 1024, VolumeConnector.S3));
        } catch(ApiException e) {
            e.printStackTrace();
        }
        Thread.sleep(4000);

        String cinderVolumeName = "CinderVol";
        System.out.println("Creating cinder volume " + cinderVolumeName +
                           ", policy: 2MB blocksize");
        VolumePolicy cinderPolicy = new VolumePolicy(2 * 1024 * 1024,
                                                     VolumeConnector.CINDER);
        config.createVolume(DOMAIN_NAME, cinderVolumeName, cinderPolicy);

        System.out.println("Attaching volume " + cinderVolumeName +
                           ", policy: 2MB blocksize");
        am.attachVolume(DOMAIN_NAME, cinderVolumeName);

        System.out.println("Creating volume 'Volume1', policy: 2MB blocksize");
        VolumePolicy volumePolicy = new VolumePolicy(2 * 1024 * 1024,
                                                     VolumeConnector.S3);
        config.createVolume(DOMAIN_NAME, VOLUME_NAME, volumePolicy);
        Thread.sleep(4000);

        List<VolumeDescriptor> volumeDescriptors = config.listVolumes(DOMAIN_NAME);
        System.out.println("Found " + volumeDescriptors.size() + " volumes");

        TxDescriptor txDesc = am.startBlobTx(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);
        System.out.println("Started transaction " + txDesc.txId);

        int length = 2 * 1024 * 1024; // volumePolicy.getMaxObjectSizeInBytes();
        byte[] putData = new byte[length];
        byte pattern = (byte)255;
        Arrays.fill(putData, pattern);

        System.out.println("Creating object '"+BLOB_NAME+"', size: " + length + " bytes");
        int offCount = 0;
        for (offCount = 0; offCount < 10; offCount++) {
            am.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, txDesc,
                          ByteBuffer.wrap(putData), length,
                          new ObjectOffset(offCount),
                          ByteBuffer.wrap(new byte[0]), false);
        }
        am.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, txDesc,
                      ByteBuffer.wrap(putData), 0, new ObjectOffset(offCount),
                      ByteBuffer.wrap(new byte[0]), true);

        ByteBuffer data = am.getBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                                         length, new ObjectOffset(0));

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
