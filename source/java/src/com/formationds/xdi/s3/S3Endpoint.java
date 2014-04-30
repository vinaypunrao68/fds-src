package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

public class S3Endpoint {
    private Xdi xdi;

    public S3Endpoint(Xdi xdi) {
        this.xdi = xdi;
    }

    public void start(int port) {
        WebApp webApp = new WebApp();
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
