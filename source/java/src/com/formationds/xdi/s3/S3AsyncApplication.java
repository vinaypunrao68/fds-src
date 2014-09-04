package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.apis.ErrorCode;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.web.toolkit.AsyncRequestExecutor;
import com.formationds.xdi.XdiAsync;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.http.HttpStatus;
import org.eclipse.jetty.http.HttpURI;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;
import sun.net.www.protocol.http.AuthenticationInfo;

import javax.servlet.AsyncContext;
import javax.servlet.ServletOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.function.Supplier;

public class S3AsyncApplication implements AsyncRequestExecutor {
    private final S3Authenticator authenticator;
    private XdiAsync.Factory xdiAsyncFactory;

    public S3AsyncApplication(XdiAsync.Factory xdi, S3Authenticator authenticator) {
        this.xdiAsyncFactory = xdi;
        this.authenticator = authenticator;
    }

    @Override
    public Optional<CompletableFuture<Void>> tryExecuteRequest(Request request, Response response) {
        HttpURI uri = request.getUri();

        AuthenticationToken token = null;
        try {
            token = authenticator.authenticate(request);
        } catch (SecurityException ex) {
            return Optional.empty(); // TODO: actually handle this instead of deferring to the sync handler
        }
        final XdiAsync xdiAsync = xdiAsyncFactory.createAuthenticated(token);

        String sanitizedPath = uri.getPath().replaceAll("^/|/$", "");
        String[] chunks = sanitizedPath.split("/");

        if(chunks.length != 2 || uri.hasQuery())
            return Optional.empty();

        String bucket = chunks[0];
        String object = chunks[1];

        Supplier<CompletableFuture<Void>> result = null;
        switch(request.getMethod()) {
            case "GET":
                result = () -> getObject(xdiAsync, bucket, object, request, response);
                break;
            case "PUT":
                result = () -> putObject(xdiAsync, bucket, object, request, response);
                break;
        }

        if(result != null) {
            AsyncContext ctx = request.startAsync();
            CompletableFuture<Void> actionChain =
                result.get()
                      .exceptionally(ex -> handleError(ex, request, response))
                      .whenComplete((r, ex) -> {
                          ctx.complete();
                      });

            return Optional.of(actionChain);
        }

        return Optional.empty();
    }

    public void appendStandardHeaders(Response response, String contentType, int responseCode) {
        // TODO: factor this somewhere common
        response.addHeader("Access-Control-Allow-Origin", "*");
        response.setHeader("Server", "Formation");
        response.setContentType(contentType);
        response.setStatus(responseCode);
    }

    public Void handleError(Throwable ex, Request request, Response response) {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        while(ex instanceof CompletionException)
            ex = ex.getCause();
        ex.printStackTrace(pw);
        try {
            if(ex instanceof ApiException) {
                ApiException apiEx = (ApiException) ex;
                if(apiEx.getErrorCode() == ErrorCode.MISSING_RESOURCE) {
                    response.sendError(HttpStatus.NOT_FOUND_404, "resource not found");
                    return null;
                }
            }
            response.sendError(HttpStatus.INTERNAL_SERVER_ERROR_500, sw.toString());
        } catch (IOException exception) {
            // do nothing
        }
        return null;
    }

    public CompletableFuture<Void> getObject(XdiAsync xdiAsync, String bucket, String object, Request request, Response response) {
        // TODO: add etags
        try {
            ServletOutputStream outputStream = response.getOutputStream();
            return xdiAsync.statBlob(S3Endpoint.FDS_S3, bucket, object).thenCompose(stat -> {
                Map<String, String> md = stat.getMetadata();
                String contentType = md.getOrDefault("Content-Type", "application/octet-stream");
                appendStandardHeaders(response, contentType, HttpStatus.OK_200);
                if(md.containsKey("etag"))
                    response.addHeader("etag", formatEtag(md.get("etag")));

                return xdiAsync.getBlobToStream(S3Endpoint.FDS_S3, bucket, object, outputStream);
            });
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    public CompletableFuture<Void> putObject(XdiAsync xdiAsync, String bucket, String object, Request request, Response response) {
        try {
            appendStandardHeaders(response, "application/octet-stream", HttpStatus.OK_200);
            CompletableFuture<XdiAsync.PutResult> putResult = xdiAsync.putBlobFromStream(S3Endpoint.FDS_S3, bucket, object, request.getInputStream());
            return putResult.thenAccept(result -> response.addHeader("etag", formatEtag(result.digest)));
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    public String formatEtag(String input) {
        return "\"" + input + "\"";
    }

    public String formatEtag(byte[] input) {
        return formatEtag(Hex.encodeHexString(input));
    }
}
