package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.XdiAsync;
import com.formationds.xdi.security.Intent;
import com.formationds.xdi.security.XdiAuthorizer;
import org.eclipse.jetty.http.HttpStatus;

import java.io.OutputStream;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncGetObject implements Function<HttpContext, CompletableFuture<Void>> {
    private XdiAsync xdiAsync;
    private S3Authenticator authenticator;
    private XdiAuthorizer authorizer;

    public AsyncGetObject(XdiAsync xdiAsync, S3Authenticator authenticator, XdiAuthorizer authorizer) {
        this.xdiAsync = xdiAsync;
        this.authenticator = authenticator;
        this.authorizer = authorizer;
    }

    @Override
    public CompletableFuture<Void> apply(HttpContext ctx) {
        String bucket = ctx.getRouteParameters().get("bucket");
        String object = ctx.getRouteParameters().get("object");

        try {
            OutputStream outputStream = ctx.getOutputStream();
            return xdiAsync.getBlobInfo(S3Endpoint.FDS_S3, bucket, object).thenCompose(blobInfo -> {
                if (!hasAccess(ctx, bucket, blobInfo.blobDescriptor.getMetadata())) {
                    return CompletableFutureUtility.exceptionFuture(new SecurityException());
                }

                Map<String, String> md = blobInfo.blobDescriptor.getMetadata();
                String contentType = md.getOrDefault("Content-Type", S3Endpoint.S3_DEFAULT_CONTENT_TYPE);
                ctx.setResponseContentType(contentType);
                ctx.setResponseStatus(HttpStatus.OK_200);
                if (md.containsKey("etag"))
                    ctx.addResponseHeader("etag", AsyncPutObject.formatEtag(md.get("etag")));
                S3UserMetadataUtility.extractUserMetadata(md).forEach((key, value) -> ctx.addResponseHeader(key, value));

                return xdiAsync.getBlobToStream(blobInfo, outputStream);
            });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    private boolean hasAccess(HttpContext context, String bucket, Map<String, String> metadata) {
        String acl = metadata.getOrDefault(PutObjectAcl.X_AMZ_ACL, PutObjectAcl.PRIVATE);

        if (!acl.equals(PutObjectAcl.PRIVATE)) {
            return true;
        }

        AuthenticationToken token = authenticator.authenticate(context);
        return authorizer.hasVolumePermission(token, bucket, Intent.read);
    }
}
