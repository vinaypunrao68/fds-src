package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

import java.util.function.Function;

public class S3Endpoint {
    public static final String FDS_S3 = "FDS_S3";
    public static final String FDS_S3_SYSTEM = "FDS_S3_SYSTEM";
    public static final String FDS_S3_SYSTEM_BUCKET_NAME = FDS_S3_SYSTEM;

    private Xdi xdi;
    private final WebApp webApp;

    public S3Endpoint(Xdi xdi) {
        this.xdi = xdi;
        webApp = new WebApp();
    }

    public void start(int port) {
        authorize(HttpMethod.GET, "/", (t) -> new ListBuckets(xdi));
        authorize(HttpMethod.PUT, "/:bucket", (t) -> new CreateBucket(xdi));
        authorize(HttpMethod.DELETE, "/:bucket", (t) -> new DeleteBucket(xdi));
        authorize(HttpMethod.GET, "/:bucket", (t) -> new ListObjects(xdi));
        authorize(HttpMethod.HEAD, "/:bucket", (t) -> new HeadBucket(xdi));

        authorize(HttpMethod.PUT, "/:bucket/:object", (t) -> new PutObject(xdi));
        authorize(HttpMethod.POST, "/:bucket", (t) -> new PostObject(xdi));
        authorize(HttpMethod.POST, "/:bucket/:object", (t) -> new PostObject(xdi));
        authorize(HttpMethod.GET, "/:bucket/:object", (t) -> new GetObject(xdi));
        authorize(HttpMethod.DELETE, "/:bucket/:object", (t) -> new DeleteObject(xdi));

        webApp.start(port);
    }

    private void authorize(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        Function<AuthenticationToken, RequestHandler> errorHandler = new S3FailureHandler(f);
        webApp.route(method, route, new S3Authenticator(errorHandler, xdi));
    }
}
