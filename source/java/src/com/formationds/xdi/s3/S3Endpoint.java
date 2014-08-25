package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

import java.util.function.Supplier;

public class S3Endpoint {
    public static final String FDS_S3 = "FDS_S3";
    public static final String FDS_S3_SYSTEM = "FDS_S3_SYSTEM";
    public static final String FDS_S3_SYSTEM_BUCKET_NAME = FDS_S3_SYSTEM;

    private Xdi xdi;
    private boolean enforceAuth;
    private final WebApp webApp;

    public S3Endpoint(Xdi xdi, boolean enforceAuth) {
        this.xdi = xdi;
        this.enforceAuth = enforceAuth;
        webApp = new WebApp();
    }

    public void start(int port) {
        authorize(HttpMethod.GET, "/", () -> new ListBuckets(xdi));
        authorize(HttpMethod.PUT, "/:bucket", () -> new CreateBucket(xdi));
        authorize(HttpMethod.DELETE, "/:bucket", () -> new DeleteBucket(xdi));
        authorize(HttpMethod.GET, "/:bucket", () -> new ListObjects(xdi));
        authorize(HttpMethod.HEAD, "/:bucket", () -> new HeadBucket(xdi));

        authorize(HttpMethod.PUT, "/:bucket/:object", () -> new PutObject(xdi));
        authorize(HttpMethod.POST, "/:bucket", () -> new PostObject(xdi));
        authorize(HttpMethod.POST, "/:bucket/:object", () -> new PostObject(xdi));
        authorize(HttpMethod.GET, "/:bucket/:object", () -> new GetObject(xdi));
        authorize(HttpMethod.DELETE, "/:bucket/:object", () -> new DeleteObject(xdi));

        webApp.start(port);
    }

    private void authorize(HttpMethod method, String route, Supplier<RequestHandler> s) {
        Supplier<RequestHandler> supplier = new S3FailureHandler(s);
        if (enforceAuth) {
            webApp.route(method, route, new S3Authenticator(supplier, xdi));
        } else {
            webApp.route(method, route, supplier);
        }
    }
}
