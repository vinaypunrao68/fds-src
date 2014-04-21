package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.LocalAmShim;

public class S3Endpoint {

    public static final String FDS_S3 = "FDS_S3";

    public static void main(String[] args) throws Exception {
        new S3Endpoint().start(9977);
    }

    public void start(int port) throws Exception {
        WebApp webApp = new WebApp();
        LocalAmShim am = new LocalAmShim();
        am.createDomain(FDS_S3);
        Xdi xdi = new Xdi(am);
        webApp.route(HttpMethod.GET, "/", () -> new ListBuckets(xdi));
        webApp.route(HttpMethod.PUT, "/:bucket", () -> new CreateBucket(xdi));
        webApp.route(HttpMethod.DELETE, "/:bucket", () -> new DeleteBucket(xdi));
        webApp.route(HttpMethod.GET, "/:bucket", () -> new ListObjects(xdi));
        webApp.route(HttpMethod.HEAD, "/:bucket", () -> new HeadBucket(xdi));

        webApp.route(HttpMethod.PUT, "/:bucket/:object", () -> new PutObject(xdi));
        webApp.route(HttpMethod.GET, "/:bucket/:object", () -> new GetObject(xdi));
        webApp.route(HttpMethod.DELETE, "/:bucket/:object", () -> new DeleteObject(xdi));


        webApp.start(port);
    }
}
