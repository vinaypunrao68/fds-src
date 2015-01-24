package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.App;
import com.formationds.spike.later.AsyncBridge;
import com.formationds.spike.later.HttpPath;
import com.formationds.spike.later.HttpPathContext;
import com.formationds.web.toolkit.*;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiAsync;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;
import org.joda.time.format.ISODateTimeFormat;

import javax.crypto.SecretKey;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class S3Endpoint {
    private final static org.apache.log4j.Logger LOG = org.apache.log4j.Logger.getLogger(S3Endpoint.class);
    private final S3Authenticator authenticator;
    public static final String FDS_S3 = "FDS_S3";
    public static final String FDS_S3_SYSTEM = "FDS_S3_SYSTEM";
    public static final String X_AMZ_COPY_SOURCE = "x-amz-copy-source";
    public static final String S3_DEFAULT_CONTENT_TYPE = "binary/octet-stream";
    private final App webApp;
    private Xdi xdi;
    private Function<AuthenticationToken, XdiAsync> xdiAsync;
    private SecretKey secretKey;

    public S3Endpoint(Xdi xdi, Function<AuthenticationToken, XdiAsync> xdiAsync, SecretKey secretKey,
                      HttpsConfiguration httpsConfiguration, HttpConfiguration httpConfiguration) {
        this.xdi = xdi;
        this.xdiAsync = xdiAsync;
        this.secretKey = secretKey;
        webApp = new App(httpConfiguration, httpsConfiguration);
        authenticator = new S3Authenticator(xdi, secretKey);
    }

    public static String formatAwsDate(DateTime dateTime) {
        return dateTime.toString(ISODateTimeFormat.dateTime());
    }

    public void start() {
        syncRoute(HttpMethod.GET, "/", (t) -> new ListBuckets(xdi, t));
        syncRoute(HttpMethod.PUT, "/:bucket", (t) -> new CreateBucket(xdi, t));
        syncRoute(HttpMethod.DELETE, "/:bucket", (t) -> new DeleteBucket(xdi, t));
        syncRoute(HttpMethod.GET, "/:bucket", (t) -> new ListObjects(xdi, t));
        syncRoute(HttpMethod.HEAD, "/:bucket", (t) -> new HeadBucket(xdi, t));

        syncRoute(new HttpPath(HttpMethod.PUT, "/:bucket/:object")
                        .withHeader(S3Endpoint.X_AMZ_COPY_SOURCE),
                (t) -> new PutObject(xdi, t));

        syncRoute(new HttpPath(HttpMethod.PUT, "/:bucket/:object")
                        .withUrlParam("uploadId"),
                (t) -> new PutObject(xdi, t));

        syncRoute(new HttpPath(HttpMethod.PUT, "/:bucket/:object")
                        .withUrlParam("acl"),
                (t) -> new PutObject(xdi, t));

        webApp.route(new HttpPath(HttpMethod.PUT, "/:bucket/:object"), ctx ->
                auth(ctx).thenCompose(xdiAsync1 -> new AsyncPutObject(xdiAsync1).apply(ctx)));

        syncRoute(HttpMethod.POST, "/:bucket", (t) -> new PostObject(xdi, t));
        syncRoute(HttpMethod.POST, "/:bucket/:object", (t) -> new PostObject(xdi, t));

        syncRoute(new HttpPath(HttpMethod.GET, "/:bucket/:object")
                        .withUrlParam("uploadId"),
                (t) -> new GetObject(xdi, t));

        webApp.route(new HttpPath(HttpMethod.GET, "/:bucket/:object"), ctx ->
                auth(ctx).thenCompose(xdiAsync -> new AsyncGetObject(xdiAsync).apply(ctx)));

        syncRoute(HttpMethod.HEAD, "/:bucket/:object", (t) -> new HeadObject(xdi, t));
        syncRoute(HttpMethod.DELETE, "/:bucket/:object", (t) -> new DeleteObject(xdi, t));

        webApp.start();
    }


    private CompletableFuture<XdiAsync> auth(HttpPathContext ctx) {
        CompletableFuture<XdiAsync> cf = new CompletableFuture<>();
        try {
            AuthenticationToken token = authenticator.authenticate(ctx.getRequest());
            cf.complete(xdiAsync.apply(token));
        } catch (Exception e) {
            cf.completeExceptionally(e);
        }

        return cf;
    }

    private void syncRoute(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        syncRoute(new HttpPath(method, route), f);
    }

    private void syncRoute(HttpPath path, Function<AuthenticationToken, RequestHandler> f) {
        AsyncBridge bridge = new AsyncBridge(new RequestHandler() {
            @Override
            public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
                try {
                    AuthenticationToken token = null;
                    token = new S3Authenticator(xdi, secretKey).authenticate(request);
                    AuthenticatedRequestContext.begin(token);
                    Function<AuthenticationToken, RequestHandler> errorHandler = new S3FailureHandler(f);
                    return errorHandler.apply(token).handle(request, routeParameters);
                } catch (SecurityException e) {
                    return new S3Failure(S3Failure.ErrorCode.AccessDenied, "Access denied", request.getRequestURI());
                } catch (Exception e) {
                    LOG.debug("Got an exception: ", e);
                    return new S3Failure(S3Failure.ErrorCode.InternalError, "Internal error", request.getRequestURI());
                } finally {
                    AuthenticatedRequestContext.complete();
                }
            }
        });

        webApp.route(path, bridge);
    }
}
