package com.formationds.xdi.s3;

import com.formationds.spike.later.HttpPathContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.XdiAsync;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.http.HttpStatus;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;

import java.util.HashMap;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncPutObject implements Function<HttpPathContext, CompletableFuture<Void>> {
    private XdiAsync xdiAsync;

    public AsyncPutObject(XdiAsync xdiAsync) {
        this.xdiAsync = xdiAsync;
    }

    public static String formatEtag(String input) {
        return "\"" + input + "\"";
    }

    public CompletableFuture<Void> apply(HttpPathContext ctx) {
        String bucket = ctx.getRouteParameters().get("bucket");
        String object = ctx.getRouteParameters().get("object");
        Request request = ctx.getRequest();
        Response response = ctx.getResponse();

        try {
            HashMap<String, String> metadata = new HashMap<>();
            if (request.getContentType() != null)
                metadata.put("Content-type", request.getContentType());
            metadata.putAll(S3UserMetadataUtility.requestUserMetadata(request));

            response.setContentType("text/html");
            response.setStatus(HttpStatus.OK_200);
            CompletableFuture<XdiAsync.PutResult> putResult = xdiAsync.putBlobFromStream(S3Endpoint.FDS_S3, bucket, object, metadata, request.getInputStream());
            return putResult.thenAccept(result -> response.addHeader("etag", formatEtag(Hex.encodeHexString(result.digest))));
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }
}
