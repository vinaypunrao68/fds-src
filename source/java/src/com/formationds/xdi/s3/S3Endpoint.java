package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.BypassAuthenticator;
import com.formationds.util.Configuration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.ToyServices;

import java.util.function.Supplier;

public class S3Endpoint {
    public static final String FDS_S3 = "FDS_S3";
    private Xdi xdi;
    private boolean enforceAuth;
    private final WebApp webApp;

    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration(args);
        ToyServices foo = new ToyServices("foo");
        foo.createDomain(FDS_S3);
        Xdi xdi = new Xdi(foo, foo, new BypassAuthenticator());
        new S3Endpoint(xdi, false).start(9999);
    }


    public S3Endpoint(Xdi xdi, boolean enforceAuth) {
        this.xdi = xdi;
        this.enforceAuth = true;
        webApp = new WebApp();
    }

    public void start(int port) {
        authorize(HttpMethod.GET, "/", () -> new ListBuckets(xdi));
        authorize(HttpMethod.PUT, "/:bucket", () -> new CreateBucket(xdi));
        authorize(HttpMethod.DELETE, "/:bucket", () -> new DeleteBucket(xdi));
        authorize(HttpMethod.GET, "/:bucket", () -> new ListObjects(xdi));
        authorize(HttpMethod.HEAD, "/:bucket", () -> new HeadBucket(xdi));

        authorize(HttpMethod.PUT, "/:bucket/:object", () -> new PutObject(xdi));
        authorize(HttpMethod.POST, "/:bucket", () -> new HtmlPostUpload(xdi));
        authorize(HttpMethod.GET, "/:bucket/:object", () -> new GetObject(xdi));
        authorize(HttpMethod.DELETE, "/:bucket/:object", () -> new DeleteObject(xdi));

        webApp.start(port);
    }

    private void authorize(HttpMethod method, String route, Supplier<RequestHandler> supplier) {
        if (enforceAuth) {
            webApp.route(method, route, new S3Authorizer(supplier));
        } else {
            webApp.route(method, route, supplier);
        }
    }
}
