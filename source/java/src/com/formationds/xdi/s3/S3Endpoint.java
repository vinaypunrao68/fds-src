package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.*;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiAsync;
import org.joda.time.DateTime;
import org.joda.time.format.ISODateTimeFormat;

import javax.crypto.SecretKey;
import java.util.function.Function;

public class S3Endpoint {
    public static final String FDS_S3 = "FDS_S3";
    public static final String FDS_S3_SYSTEM = "FDS_S3_SYSTEM";
    public static final String X_AMZ_COPY_SOURCE = "x-amz-copy-source";
    public static final String S3_DEFAULT_CONTENT_TYPE = "binary/octet-stream";

    private Xdi xdi;
    private XdiAsync.Factory xdiAsync;
    private SecretKey secretKey;
    private HttpsConfiguration httpsConfiguration;
    private HttpConfiguration httpConfiguration;
    private final WebApp webApp;

    public S3Endpoint(Xdi xdi, XdiAsync.Factory xdiAsync, SecretKey secretKey, HttpsConfiguration httpsConfiguration, HttpConfiguration httpConfiguration) {
        this.xdi = xdi;
        this.xdiAsync = xdiAsync;
        this.secretKey = secretKey;
        this.httpsConfiguration = httpsConfiguration;
        this.httpConfiguration = httpConfiguration;

        webApp = new WebApp();
    }

    public void start() {
        authenticate(HttpMethod.GET, "/", (t) -> new ListBuckets(xdi, t));
        authenticate(HttpMethod.PUT, "/:bucket", (t) -> new CreateBucket(xdi, t));
        authenticate(HttpMethod.DELETE, "/:bucket", (t) -> new DeleteBucket(xdi, t));
        authenticate(HttpMethod.GET, "/:bucket", (t) -> new ListObjects(xdi, t));
        authenticate(HttpMethod.HEAD, "/:bucket", (t) -> new HeadBucket(xdi, t));
        authenticate(HttpMethod.PUT, "/:bucket/:object", (t) -> new PutObject(xdi, t));
        authenticate(HttpMethod.POST, "/:bucket", (t) -> new PostObject(xdi, t));
        authenticate(HttpMethod.POST, "/:bucket/:object", (t) -> new PostObject(xdi, t));
        authenticate(HttpMethod.GET, "/:bucket/:object", (t) -> new GetObject(xdi, t));
        authenticate(HttpMethod.HEAD, "/:bucket/:object", (t) -> new HeadObject(xdi, t));
        authenticate(HttpMethod.DELETE, "/:bucket/:object", (t) -> new DeleteObject(xdi, t));

        webApp.addAsyncExecutor(new S3AsyncApplication(xdiAsync, new S3Authenticator(xdi, secretKey)));

        webApp.start(httpConfiguration, httpsConfiguration);
    }

    private void authenticate(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        Function<AuthenticationToken, RequestHandler> errorHandler = new S3FailureHandler(f);
        webApp.route(method, route, () -> (r, rp) -> {
            try {
                AuthenticationToken token = new S3Authenticator(xdi, secretKey).authenticate(r);
                return errorHandler.apply(token).handle(r, rp);
            } catch (SecurityException e) {
                return new S3Failure(S3Failure.ErrorCode.AccessDenied, "Access denied", r.getRequestURI());
            }
        });
    }

    public static String formatAwsDate(DateTime dateTime) {
        return dateTime.toString(ISODateTimeFormat.dateTime());
    }
}
