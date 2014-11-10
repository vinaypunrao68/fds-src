package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.util.Size;
import com.formationds.util.SizeUnit;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public class SmokeTest {

    public static final String DOMAIN_NAME = "FDS";
    public static final String VOLUME_NAME = "S3Vol";
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
        // TFramedTransport tfTrans = new TFramedTransport(socket);
        // tfTrans.open();
        socket.open();
        // AmService.Iface am = new AmService.Client(new TBinaryProtocol(tfTrans));
        AmService.Iface am = new AmService.Client(new TBinaryProtocol(socket));
        TSocket omTransport = new TSocket("localhost", 9090);
        omTransport.open();
        
        ConfigurationService.Iface config = new ConfigurationService.Client(new TBinaryProtocol(omTransport));

        String cinderVolumeName = "CinderVol";
        System.out.println("Creating volume " + cinderVolumeName +
                           ", policy: 4KB blocksize");
        VolumeSettings cinderPolicy = new VolumeSettings(4 * 1024, VolumeType.BLOCK, new Size(20, SizeUnit.GB).totalBytes(), 0);
        config.createVolume(DOMAIN_NAME, cinderVolumeName, cinderPolicy, 0);

        System.out.println("Attaching volume " + cinderVolumeName);
        am.attachVolume(DOMAIN_NAME, cinderVolumeName);

        System.out.println("Creating volume " + VOLUME_NAME + ", policy: 2MB blocksize");
        VolumeSettings volumePolicy = new VolumeSettings(2 * 1024 * 1024, VolumeType.OBJECT, 0, 0);
        config.createVolume(DOMAIN_NAME, VOLUME_NAME, volumePolicy, 0);
        Thread.sleep(4000);

        List<VolumeDescriptor> volumeDescriptors = config.listVolumes(DOMAIN_NAME);
        System.out.println("Found " + volumeDescriptors.size() + " volumes");

        TxDescriptor txDesc = am.startBlobTx(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, 0);
        System.out.println("Started transaction " + txDesc.txId);

        int length = volumePolicy.getMaxObjectSizeInBytes();
        byte[] putData = new byte[length];
        byte pattern = (byte)255;
        Arrays.fill(putData, pattern);

        System.out.println("Creating object '"+BLOB_NAME+"', size: " + length + " bytes");
        int offCount = 0;
        for (offCount = 0; offCount < 10; offCount++) {
            am.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, txDesc,
                          ByteBuffer.wrap(putData), length,
                          new ObjectOffset(offCount), false);
        }
        am.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, txDesc,
                      ByteBuffer.wrap(putData), 0, new ObjectOffset(offCount), true);

        ByteBuffer data = am.getBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME,
                                         length, new ObjectOffset(0));

        System.out.println("setting blob meta");
        HashMap<String,String> meta = new HashMap<String,String>();

        meta.put("company","fds");
        am.updateMetadata(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, txDesc, meta);
        System.out.println("done setting metadata");
        
        System.out.println("stating blob");
        System.out.println(am.statBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME));
        
        System.out.println("stating volume");
        System.out.println(config.statVolume(DOMAIN_NAME, VOLUME_NAME));
        System.out.println(am.volumeStatus(DOMAIN_NAME, VOLUME_NAME));

        System.out.println("volume contents");
        System.out.println(am.volumeContents(DOMAIN_NAME, VOLUME_NAME, 0, 0));

        System.out.println("deleting blob");
        am.deleteBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);

        System.out.println("stating volume");
        System.out.println(config.statVolume(DOMAIN_NAME, VOLUME_NAME));
        System.out.println(am.volumeStatus(DOMAIN_NAME, VOLUME_NAME));
        
        System.out.println("volume contents");
        System.out.println(am.volumeContents(DOMAIN_NAME, VOLUME_NAME, 0, 0));

        System.out.println("volume list");
        System.out.println(config.listVolumes(DOMAIN_NAME));

        System.out.println("deleting volume");
        config.deleteVolume(DOMAIN_NAME, VOLUME_NAME);
        System.out.println("All done.");
    }
}
