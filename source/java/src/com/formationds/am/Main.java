package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.Configuration;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.LocalAmShim;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.shim.AmShim;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;

public class Main {
    public static final String FDS_S3 = "FDS_S3";

    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration(args);
        NativeAm.startAm(args);

        TSocket transport = new TSocket("localhost", 9988);
        transport.open();
        AmShim.Client am = new AmShim.Client(new TBinaryProtocol(transport));

        Xdi xdi = new Xdi(am);

        new S3Endpoint(xdi).start(9977);
    }
}
