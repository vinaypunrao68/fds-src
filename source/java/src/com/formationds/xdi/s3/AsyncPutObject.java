package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpPathContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.XdiAsync;
import com.formationds.xdi.security.Intent;
import com.formationds.xdi.security.XdiAuthorizer;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.http.HttpStatus;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;

import javax.servlet.ServletInputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncPutObject implements Function<HttpPathContext, CompletableFuture<Void>> {
    private XdiAsync xdiAsync;
    private XdiAuthorizer authorizer;
    private S3Authenticator authenticator;

    public AsyncPutObject(XdiAsync xdiAsync, S3Authenticator authenticator, XdiAuthorizer authorizer) {
        this.xdiAsync = xdiAsync;
        this.authenticator = authenticator;
        this.authorizer = authorizer;
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
            response.setContentType("text/html");
            response.setStatus(HttpStatus.OK_200);
            ServletInputStream inputStream = request.getInputStream();
            return filterAccess(request, bucket, object)
                    .thenApply(metadata -> updateMetadata(request, metadata))
                    .thenCompose(metadata -> xdiAsync.putBlobFromStream(S3Endpoint.FDS_S3, bucket, object, metadata, inputStream))
                    .thenAccept(result -> response.addHeader("etag", formatEtag(Hex.encodeHexString(result.digest))));
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    private Map<String, String> updateMetadata(Request request, Map<String, String> metadata) {
        if (request.getContentType() != null)
            metadata.put("Content-type", request.getContentType());
        metadata.putAll(S3UserMetadataUtility.requestUserMetadata(request));
        return metadata;
    }


    private CompletableFuture<Map<String, String>> filterAccess(Request request, String bucket, String key) {
        return xdiAsync.statBlob(S3Endpoint.FDS_S3, bucket, key)
                .thenApply(bd -> bd.getMetadata())
                .exceptionally(t -> new HashMap<String, String>()) // This blob might not exist, so we use empty metadata as a filter
                .thenCompose(metadata -> {
                    CompletableFuture<Map<String, String>> cf = new CompletableFuture<Map<String, String>>();

                    String acl = metadata.getOrDefault(PutObjectAcl.X_AMZ_ACL, PutObjectAcl.PRIVATE);

                    if (acl.equals(PutObjectAcl.PUBLIC_READ_WRITE)) {
                        cf.complete(metadata);
                    } else {
                        AuthenticationToken token = authenticator.authenticate(request);
                        if (authorizer.hasVolumePermission(token, bucket, Intent.write)) {
                            cf.complete(metadata);
                        } else {
                            cf.completeExceptionally(new SecurityException());
                        }
                    }

                    return cf;
                });
    }
}
