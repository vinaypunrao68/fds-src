package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

public class Main {

    public static final String FDS_S3 = "FDS_S3";

    public static void main(String[] args) throws Exception {
        WebApp webApp = new WebApp();
        Xdi xdi = new Xdi(FDS_S3);
        webApp.route(HttpMethod.GET, "/", () -> new ListBuckets(xdi));
    }
}
