package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.am.Main;
import com.formationds.util.Configuration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.LocalAmShim;
import com.formationds.xdi.shim.AmShim;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;

public class S3Endpoint {
    private Xdi xdi;

    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration(args);
        TSocket transport = new TSocket("localhost", 9988);
        transport.open();
        AmShim.Client am = new AmShim.Client(new TBinaryProtocol(transport));
        // am.createDomain(Main.FDS_S3);
        Xdi xdi = new Xdi(am);
        new S3Endpoint(xdi).start(9977);
    }

    public S3Endpoint(Xdi xdi) {
        this.xdi = xdi;
    }

    public void start(int port) throws Exception {
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
