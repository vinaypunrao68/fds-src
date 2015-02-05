package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpPathContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.XdiAsync;
import com.formationds.xdi.security.Intent;
import com.formationds.xdi.security.XdiAuthorizer;
import org.eclipse.jetty.http.HttpStatus;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;

import javax.servlet.ServletOutputStream;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncGetObject implements Function<HttpPathContext, CompletableFuture<Void>> {
    private XdiAsync xdiAsync;
    private S3Authenticator authenticator;
    private XdiAuthorizer authorizer;

    public AsyncGetObject(XdiAsync xdiAsync, S3Authenticator authenticator, XdiAuthorizer authorizer) {
        this.xdiAsync = xdiAsync;
        this.authenticator = authenticator;
        this.authorizer = authorizer;
    }

    @Override
    public CompletableFuture<Void> apply(HttpPathContext ctx) {
        String bucket = ctx.getRouteParameters().get("bucket");
        String object = ctx.getRouteParameters().get("object");

        Response response = ctx.getResponse();

        try {
            ServletOutputStream outputStream = response.getOutputStream();
            return xdiAsync.getBlobInfo(S3Endpoint.FDS_S3, bucket, object).thenCompose(blobInfo -> {
                if (!hasAccess(ctx.getRequest(), bucket, blobInfo.blobDescriptor.getMetadata())) {
                    return CompletableFutureUtility.exceptionFuture(new SecurityException());
                }

                Map<String, String> md = blobInfo.blobDescriptor.getMetadata();
                String contentType = md.getOrDefault("Content-Type", S3Endpoint.S3_DEFAULT_CONTENT_TYPE);
                response.setContentType(contentType);
                response.setStatus(HttpStatus.OK_200);
                if (md.containsKey("etag"))
                    response.addHeader("etag", AsyncPutObject.formatEtag(md.get("etag")));
                S3UserMetadataUtility.extractUserMetadata(md).forEach((key, value) -> response.addHeader(key, value));

                return xdiAsync.getBlobToStream(blobInfo, outputStream);
            });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    private boolean hasAccess(Request request, String bucket, Map<String, String> metadata) {
        String acl = metadata.getOrDefault(PutObjectAcl.X_AMZ_ACL, PutObjectAcl.PRIVATE);

        if (!acl.equals(PutObjectAcl.PRIVATE)) {
            return true;
        }

        AuthenticationToken token = authenticator.authenticate(request);
        return authorizer.hasVolumePermission(token, bucket, Intent.read);
    }
}
