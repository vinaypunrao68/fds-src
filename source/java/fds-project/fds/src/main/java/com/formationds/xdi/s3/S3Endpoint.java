package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.AsyncWebapp;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.HttpPath;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.spike.later.pathtemplate.PathTemplate;
import com.formationds.web.toolkit.*;
import com.formationds.xdi.AsyncStreamer;
import com.formationds.xdi.Xdi;
import io.netty.util.internal.chmv8.ConcurrentHashMapV8;
import org.joda.time.DateTime;
import org.joda.time.format.ISODateTimeFormat;

import javax.crypto.SecretKey;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.function.BiFunction;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.IntStream;

public class S3Endpoint {
    private final static org.apache.log4j.Logger LOG = org.apache.log4j.Logger.getLogger(S3Endpoint.class);
    private final S3Authenticator authenticator;
    public static final String FDS_S3 = "FDS_S3";
    public static final String FDS_S3_SYSTEM = "FDS_S3_SYSTEM";
    public static final String X_AMZ_COPY_SOURCE = "x-amz-copy-source";
    public static final String S3_DEFAULT_CONTENT_TYPE = "binary/octet-stream";
    private final AsyncWebapp webApp;
    private Xdi xdi;
    private Supplier<AsyncStreamer> xdiAsync;
    private SecretKey secretKey;

    public S3Endpoint(Xdi xdi, Supplier<AsyncStreamer> xdiAsync, SecretKey secretKey,
                      HttpsConfiguration httpsConfiguration, HttpConfiguration httpConfiguration) {
        this.xdi = xdi;
        this.xdiAsync = xdiAsync;
        this.secretKey = secretKey;
        webApp = new AsyncWebapp(httpConfiguration, httpsConfiguration);
        authenticator = new S3Authenticator(xdi.getAuthorizer(), secretKey);
    }

    public static String formatAwsDate(DateTime dateTime) {
        return dateTime.toString(ISODateTimeFormat.dateTime());
    }

    public void start() {
        setupNotImplementedRoutes();


        syncRoute(HttpMethod.GET, "/", (t) -> new ListBuckets(xdi, t));

        syncRoute(new HttpPath(HttpMethod.PUT, "/:bucket")
                        .withUrlParam("acl")
                        .withHeader("x-amz-acl"),
                (t) -> new PutBucketAcl(xdi, t));

        syncRoute(HttpMethod.PUT, "/:bucket", (t) -> new CreateBucket(xdi, t));
        syncRoute(HttpMethod.DELETE, "/:bucket", (t) -> new DeleteBucket(xdi, t));
        syncRoute(HttpMethod.GET, "/:bucket", (t) -> new ListObjects(xdi, t));
        syncRoute(HttpMethod.HEAD, "/:bucket", (t) -> new HeadBucket(xdi, t));

        // TODO: this is a hack to handle the fact object paths can have variable length
        for(int i = 1; i <= 50; i++) {
            StringBuilder sb = new StringBuilder();
            sb.append("/:bucket");
            for(int j = 0; j < i; j++) {
                sb.append("/:object" + j);
            }

            HttpPath path = new HttpPath().withPath(sb.toString());
            setupObjectRoutes(path);
        }

        // this handles object paths of any length (but more slowly than the above)
        setupObjectRoutes(new HttpPath().withPredicate(this::isObjectPath));

        webApp.start();
    }

    // TODO: remove object pathing hack
    //       our routing system doesn't handle variable length paths gracefully
    //       so we have a bunch of routes for this, for path lengths from 1-50
    //       as well as a path

    private boolean isObjectPath(HttpContext ctx) {
        String[] parts = ctx.getRequestURI().replaceAll("^/", "") .split("/", 2);
        return parts.length == 2;
    }

    private void syncObjectRoute(HttpPath path, Function<AuthenticationToken, SyncRequestHandler> f) {
        syncRoute(path, tkn -> ctx -> f.apply(tkn).handle(mergeObjectPath(ctx)));
    }

    private HttpContext mergeObjectPath(HttpContext context) {
        Map<String, String> routeParameters = context.getRouteParameters();
        Map<String, String> rp = new HashMap<>(routeParameters);

        String p = context.getRequestURI().replaceAll("^/+", "").replaceAll("%2F", "/");
        String[] parts = p.split("/", 2);

        rp.put("object", parts[1]);
        rp.put("bucket", parts[0]);
        return context.withRouteParameters(rp);
    }

    private void setupObjectRoutes(HttpPath path) {
        // unimplemented
        notImplementedObjectSubResource("cors", path.clone(), HttpMethod.OPTIONS);
        notImplementedObjectSubResource("lifecycle", path.clone().withUrlParam("restore"), HttpMethod.POST);

        syncObjectRoute(path.clone().withMethod(HttpMethod.GET)
                        .withUrlParam("uploadId"),
                (t) -> new MultiPartListParts(xdi, t));

        webApp.route(path.clone().withMethod(HttpMethod.GET), ctx ->
                executeAsync(mergeObjectPath(ctx), new AsyncGetObject(xdi)));

        syncObjectRoute(path.clone().withMethod(HttpMethod.PUT)
                        .withHeader(S3Endpoint.X_AMZ_COPY_SOURCE),
                (t) -> new PutObject(xdi, t));

        syncObjectRoute(path.clone().withMethod(HttpMethod.PUT)
                        .withUrlParam("uploadId"),
                (t) -> new PutObject(xdi, t));

        syncObjectRoute(path.clone().withMethod(HttpMethod.PUT)
                        .withUrlParam("acl")
                        .withHeader("x-amz-acl"),
                (t) -> new PutObjectAcl(xdi, t));

        webApp.route(path.clone().withMethod(HttpMethod.PUT), ctx ->
                executeAsync(mergeObjectPath(ctx), new AsyncPutObject(xdi)));

        syncObjectRoute(path.clone().withMethod(HttpMethod.POST)
                .withUrlParam("delete"), (t) -> new DeleteMultipleObjects(xdi, t));

        syncObjectRoute(path.clone().withMethod(HttpMethod.POST)
                .withUrlParam("uploads"), (t) -> new MultiPartUploadInitiate(xdi, t));

        syncObjectRoute(path.clone().withMethod(HttpMethod.POST)
                .withUrlParam("uploadId"), (t) -> new MultiPartUploadComplete(xdi, t));

        syncObjectRoute(path.clone().withMethod(HttpMethod.POST), (t) -> new PostObjectUpload(xdi, t));

        syncObjectRoute(path.clone().withMethod(HttpMethod.HEAD), (t) -> new HeadObject(xdi, t));
        syncObjectRoute(path.clone().withMethod(HttpMethod.DELETE), (t) -> new DeleteObject(xdi, t));
    }

    // end object pathing hack

    private void setupNotImplementedRoutes() {
        notImplementedBucketSubResources("cors", "lifecycle", "policy", "replication", "tagging", "website", "location",
                "notification", "versions", "versioning", "logging", "requestPayment");

        notImplementedBucketSubResource("bucket uploads", p -> p.withUrlParam("uploads"), HttpMethod.GET);
    }

    private void notImplementedBucketSubResources(String ... subresourceStrings) {
        for(String subresource : subresourceStrings)
            notImplementedBucketSubResource("bucket " + subresource, p -> p.withUrlParam(subresource));
    }

    private void notImplementedBucketSubResource(String feature, Function<HttpPath, HttpPath> path) {
        notImplementedBucketSubResource(feature, path, HttpMethod.PUT, HttpMethod.DELETE, HttpMethod.GET);
    }

    private void notImplementedObjectSubResource(String feature, HttpPath path, HttpMethod...methods) {
        for(HttpMethod method : methods)
            webApp.route(path.clone().withMethod(method), ctx -> notImplementedHandler(ctx, feature));
    }

    private void notImplementedBucketSubResource(String feature, Function<HttpPath, HttpPath> path, HttpMethod...methods) {
        for(HttpMethod method : methods)
            webApp.route(path.apply(new HttpPath(method, "/:bucket")), ctx -> notImplementedHandler(ctx, feature));
    }

    private CompletableFuture<Void> notImplementedHandler(HttpContext ctx, String featureName) {
        LOG.error("Attempt to use non-implemented feature '" + featureName + "'");
        S3Failure s3Failure = new S3Failure(S3Failure.ErrorCode.InternalError, "This feature is not implemented by FDS [" + featureName + "]", ctx.getRequestURI());
        s3Failure.renderTo(ctx);
        return CompletableFuture.completedFuture(null);
    }

    private CompletableFuture<Void> executeAsync(HttpContext ctx, BiFunction<HttpContext, AuthenticationToken, CompletableFuture<Void>> function) {
        CompletableFuture<Void> cf = null;
        try {
            AuthenticationToken token = new S3Authenticator(xdi.getAuthorizer(), secretKey).authenticate(ctx);
            cf = function.apply(ctx, token);
        } catch (Exception e) {
            cf.completeExceptionally(e);
        }

        return cf.exceptionally(e -> {
            String requestUri = ctx.getRequestURI();
            Resource resource = new TextResource("");
            if (e.getCause() instanceof SecurityException) {
                resource = new S3Failure(S3Failure.ErrorCode.AccessDenied, "Access denied", requestUri);
            } else if (e.getCause() instanceof ApiException && ((ApiException) e.getCause()).getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                resource = new S3Failure(S3Failure.ErrorCode.NoSuchKey, "No such key", requestUri);
            } else {
                LOG.error("Error executing " + ctx.getRequestMethod() + " " + requestUri, e);
                resource = new S3Failure(S3Failure.ErrorCode.InternalError, "Internal error", requestUri);
            }
            resource.renderTo(ctx);
            return null;
        });
    }

    private void syncRoute(HttpMethod method, String route, Function<AuthenticationToken, SyncRequestHandler> f) {
        syncRoute(new HttpPath(method, route), f);
    }

    private void syncRoute(HttpPath path, Function<AuthenticationToken, SyncRequestHandler> f) {
        webApp.route(path, ctx -> {
                    CompletableFuture cf = new CompletableFuture();
                    Resource resource = new TextResource("");
                    try {
                        AuthenticationToken token = new S3Authenticator(xdi.getAuthorizer(), secretKey).authenticate(ctx);
                        AuthenticatedRequestContext.begin(token);
                        Function<AuthenticationToken, SyncRequestHandler> errorHandler = new S3FailureHandler(f);
                        resource = errorHandler.apply(token).handle(ctx);
                        cf.complete(null);
                    } catch (Throwable t) {
                        if (t instanceof ExecutionException) {
                            t = t.getCause();
                        }

                        if (t instanceof SecurityException) {
                            resource = new S3Failure(S3Failure.ErrorCode.AccessDenied, "Access denied", ctx.getRequestURI());
                        } else {
                            LOG.debug("Got an exception: ", t);
                            resource = new S3Failure(S3Failure.ErrorCode.InternalError, "Internal error", ctx.getRequestURI());
                        }
                        cf.completeExceptionally(t);
                    } finally {
                        AuthenticatedRequestContext.complete();
                        resource.renderTo(ctx);
                    }
                    return cf;
                });
    }
}
