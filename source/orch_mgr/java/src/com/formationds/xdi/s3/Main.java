package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.LocalAmShim;

public class Main {

    public static final String FDS_S3 = "FDS_S3";

    public static void main(String[] args) throws Exception {
        LocalAmShim shim = new LocalAmShim();
        Xdi xdi = new Xdi(FDS_S3, shim);
        WebApp webApp = new WebApp();
        webApp.route(HttpMethod.PUT, "/:bucket", () -> new CreateBucket(xdi));
        webApp.route(HttpMethod.HEAD, "/:bucket", () -> new StatBucket(xdi));
    }



}
