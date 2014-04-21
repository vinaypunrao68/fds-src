package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.Configuration;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.LocalAmShim;
import com.formationds.xdi.s3.S3Endpoint;

public class Main {
    public static final String FDS_S3 = "FDS_S3";

    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration(args);
        NativeAm.startAm(args);

        LocalAmShim am = new LocalAmShim();
        am.createDomain(FDS_S3);
        Xdi xdi = new Xdi(am);

        new S3Endpoint(xdi).start(9977);
    }
}
