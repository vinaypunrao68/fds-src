package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.apis.ErrorCode;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.web.toolkit.AsyncRequestExecutor;
import com.formationds.xdi.XdiAsync;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.http.HttpStatus;
import org.eclipse.jetty.http.HttpURI;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;
import org.eclipse.jetty.util.MultiMap;
import sun.net.www.protocol.http.AuthenticationInfo;

import javax.servlet.AsyncContext;
import javax.servlet.ServletOutputStream;
import java.io.*;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.function.Supplier;

public class S3AsyncApplication implements AsyncRequestExecutor {
    private final S3Authenticator authenticator;
    private XdiAsync.Factory xdiAsyncFactory;
    private AsyncRequestStatistics.Aggregate aggregateStats;
    private final LinkedList<AsyncRequestStatistics> requestStatisticsWindow;

    public S3AsyncApplication(XdiAsync.Factory xdi, S3Authenticator authenticator) {
        this.xdiAsyncFactory = xdi;
        this.authenticator = authenticator;
        this.aggregateStats = new AsyncRequestStatistics.Aggregate();
        this.requestStatisticsWindow = new LinkedList<>();
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

        AsyncRequestStatistics requestStatistics = new AsyncRequestStatistics();
        final XdiAsync xdiAsync = xdiAsyncFactory.createAuthenticated(token)
                .withStats(requestStatistics);

        String sanitizedPath = uri.getPath().replaceAll("^/|/$", "");
        String[] chunks = sanitizedPath.split("/");

        Supplier<CompletableFuture<Void>> result = null;

        if(chunks.length == 3 && chunks[0].equals("diagnostic") && chunks[1].equals("async")) {
            if(chunks[2].equals("stats") && request.getMethod().equals("GET")) {
                result = () -> requestStatistics.time("getStatistics", getStatistics(request, response));
            }
        } else if(chunks.length == 2 && !uri.hasQuery()){
            String bucket = chunks[0];
            String object = chunks[1];

            switch (request.getMethod()) {
                case "GET":
                    result = () -> requestStatistics.time("serviceGetObject", getObject(xdiAsync, bucket, object, request, response));
                    break;
                case "PUT":
                    // route copy to sync for now
                    if(request.getHeader(S3Endpoint.X_AMZ_COPY_SOURCE) != null)
                        return Optional.empty();
                    result = () -> requestStatistics.time("servicePutObject", putObject(xdiAsync, bucket, object, request, response));
                    break;
            }
        }

        if(result != null) {
            requestStatistics.enable();
            AsyncContext ctx = request.startAsync();
            CompletableFuture<Void> actionChain =
                result.get()
                      .exceptionally(ex -> handleError(ex, request, response))
                      .whenComplete((r, ex) -> {
                          ctx.complete();
                          aggregateStatistics(requestStatistics);
                      });

            return Optional.of(actionChain);
        }

        return Optional.empty();
    }

    private void aggregateStatistics(AsyncRequestStatistics stats) {
        synchronized (requestStatisticsWindow) {
            aggregateStats.add(stats);
            requestStatisticsWindow.add(stats);
            if(requestStatisticsWindow.size() > 100)
                requestStatisticsWindow.remove();
        }
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
            return xdiAsync.getBlobInfo(S3Endpoint.FDS_S3, bucket, object).thenCompose(blobInfo -> {
                Map<String, String> md = blobInfo.blobDescriptor.getMetadata();
                String contentType = md.getOrDefault("Content-Type", S3Endpoint.S3_DEFAULT_CONTENT_TYPE);
                appendStandardHeaders(response, contentType, HttpStatus.OK_200);
                if(md.containsKey("etag"))
                    response.addHeader("etag", formatEtag(md.get("etag")));

                return xdiAsync.getBlobToStream(blobInfo, outputStream);
            });
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    public CompletableFuture<Void> putObject(XdiAsync xdiAsync, String bucket, String object, Request request, Response response) {
        try {
            HashMap<String, String> metadata = new HashMap<>();
            if(request.getContentType() != null)
                metadata.put("Content-type", request.getContentType());

            appendStandardHeaders(response, "text/html", HttpStatus.OK_200);
            CompletableFuture<XdiAsync.PutResult> putResult = xdiAsync.putBlobFromStream(S3Endpoint.FDS_S3, bucket, object, metadata, request.getInputStream());
            return putResult.thenAccept(result -> response.addHeader("etag", formatEtag(result.digest)));
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    public CompletableFuture<Void> getStatistics(Request request, Response response) {
        try {
            response.setContentType("text/html");
            StringBuilder stringBuilder = new StringBuilder();
            OutputStreamWriter writer = new OutputStreamWriter(response.getOutputStream());
            synchronized (requestStatisticsWindow) {
                stringBuilder.append("Aggregate Stats:\n\n");
                aggregateStats.output(stringBuilder);
                stringBuilder.append("\n\n");

                stringBuilder.append("Lifecycle for last " + requestStatisticsWindow.size() + " requests\n\n");
                for(AsyncRequestStatistics windowEntry : requestStatisticsWindow) {
                    windowEntry.outputRequestLifecycle(stringBuilder);
                    stringBuilder.append("\n");
                }
            }

            writer.write(stringBuilder.toString());
            writer.flush();
            return CompletableFuture.completedFuture(null);
        } catch(Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    public String formatEtag(String input) {
        return "\"" + input + "\"";
    }

    public String formatEtag(byte[] input) {
        return formatEtag(Hex.encodeHexString(input));
    }
}
