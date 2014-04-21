package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.Configuration;
import com.formationds.xdi.s3.S3Endpoint;

public class Main {
    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration(args);
        NativeAm.startAm(args);
        new S3Endpoint().start(9977);
    }
}
