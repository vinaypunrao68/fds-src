package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.util.Size;
import com.formationds.util.SizeUnit;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.joda.time.DateTime;
import org.joda.time.Interval;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Random;



public class AmServiceSmokeTest {

    public static final String DOMAIN_NAME = "FDS";
    public static final String VOLUME_NAME = "smoke_volume2";
    public static final String BLOB_NAME   = "someBytes.bin";
    public static byte[] glData = new byte[5 * 1024 * 1024];
    private static int totalPuts = 0;
    private static long totalStartBlobTx = 0;
    private static long totalUpdateBlob = 0;
    private static TxDescriptor gl_txDesc = new TxDescriptor();

    private static void initData() {
        for (int i = 0; i < glData.length; i++) {
            glData[i] = (byte)i;
        }
    }

    private static void doOnePut(AmService.Iface am) throws Exception {
        String blobName = BLOB_NAME + totalPuts;

        long start = System.currentTimeMillis();
        TxDescriptor txDesc = am.startBlobTx(DOMAIN_NAME, VOLUME_NAME, blobName);
        long end = System.currentTimeMillis();
        totalStartBlobTx += end - start;

        byte[] putData = glData;
        int length = 4096;
        Random rand = new Random();
        int offset = rand.nextInt(putData.length - length);

        totalPuts++;
        if ((totalPuts % 1000) == 0) {
            System.out.println(totalPuts + ": Started transaction " + gl_txDesc.txId);
            System.out.println("Creating object '" + blobName + "', size: " +
                               length + " bytes, from data offset: " + offset);
        }

        start = System.currentTimeMillis();
        am.updateBlob(DOMAIN_NAME, VOLUME_NAME, blobName, gl_txDesc,
                      ByteBuffer.wrap(putData, offset, length), length,
                      new ObjectOffset(0), false);
        end = System.currentTimeMillis();
        totalUpdateBlob += end - start;
    }

    private static AmService.Iface getNewAmService(String host, int port)
                                                                    throws Exception {
        TSocket socket = new TSocket(host, port);
        socket.open();
        AmService.Iface am = new AmService.Client(new TBinaryProtocol(socket));

        return am;
    }

    public static void main(String[] args) throws Exception {
        String host="localhost";
        int port = 9988;
        int count = 1;
        if (args.length == 3) {
            host = args[0];
            port = Integer.parseInt(args[1]); 
            count = Integer.parseInt(args[2]);
        } else if (args.length > 0) {
            System.out.println("Usage: AmServiceSmokeTest <host> <port> <req_count>");
            return;
        }

        /* initialize the data array */
        initData();

        AmService.Iface am = getNewAmService(host, port);

        TxDescriptor txDesc = am.startBlobTx(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME);
        
        System.out.println("Starting test...");
        /* run the test */
        DateTime start = DateTime.now();
        for (int i = 0; i < count; i++) {
            doOnePut(am);

/*
            am.updateBlob(DOMAIN_NAME, VOLUME_NAME, BLOB_NAME, txDesc,
                          ByteBuffer.wrap(glData, 0, 4096), 4096,
                          new ObjectOffset(0), false);
                          */
        }
        DateTime end = DateTime.now();
        Interval elapsed = new Interval(start, end);
        System.out.println("Time elapsed in seconds: " + (elapsed.toDurationMillis() / 1000));
        System.out.println("start blob tx: " + (totalStartBlobTx));
        System.out.println("update Blob: " + (totalUpdateBlob));
        System.out.println("IOPS: " + count / (elapsed.toDurationMillis() / 1000));
        System.out.println("All done.");
    }
}
