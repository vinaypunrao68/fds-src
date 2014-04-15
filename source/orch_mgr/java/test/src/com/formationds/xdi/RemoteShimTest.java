package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.Uuid;
import com.formationds.xdi.shim.VolumePolicy;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.List;

public class RemoteShimTest {
    @Test
    public void testConnect() throws Exception {
        AmShim.Client client = new AmShim.Client(new TBinaryProtocol(new TSocket("10.1.14.39", 9988)));

        String domainName = "myDomain";
        String volumeName = "myVolume";
        String blobName = "myBlob";
        client.createVolume(domainName, volumeName, new VolumePolicy(8));
        Uuid uuid = client.startBlobTx(domainName, volumeName, blobName);
        byte[] contents = new byte[]{1, 2, 3, 4};
        client.updateBlob(domainName, volumeName, blobName, uuid, ByteBuffer.wrap(contents), 4, 0);
        client.commit(uuid);
        List<String> strings = client.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0);
        for (String string : strings) {
            System.out.println(string);
        }
    }
}
