package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.io.BlobSpecifier;
import org.eclipse.jetty.http.HttpStatus;

import java.io.OutputStream;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiFunction;

public class AsyncGetObject implements BiFunction<HttpContext, AuthenticationToken, CompletableFuture<Void>> {

    private Xdi xdi;

    public AsyncGetObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public CompletableFuture<Void> apply(HttpContext ctx, AuthenticationToken token) {
        String bucket = ctx.getRouteParameters().get("bucket");
        String object = ctx.getRouteParameters().get("object");

        try {
            return xdi.getBlobInfo(token, S3Endpoint.FDS_S3, bucket, S3Namespace.user().blobName(object)).thenCompose(blobInfo -> {

                Map<String, String> md = blobInfo.getBlobDescriptor().getMetadata();


                String contentType = md.getOrDefault("Content-Type", S3Endpoint.S3_DEFAULT_CONTENT_TYPE);
                ctx.setResponseContentType(contentType);
                ctx.setResponseStatus(HttpStatus.OK_200);

                if (md.containsKey("etag"))
                    ctx.addResponseHeader("etag", AsyncPutObject.formatEtag(md.get("etag")));

                S3MetadataUtility.extractUserMetadata(md).forEach((key, value) -> ctx.addResponseHeader(key, value));

                List<MultipartUpload.PartInfo> partInfoList = MultipartUpload.getPartInfoList(blobInfo.getBlobDescriptor());
                long length = partInfoList == null ? blobInfo.getBlobDescriptor().byteCount : MultipartUpload.PartInfo.computeLength(partInfoList);
                ctx.addResponseHeader("Content-Length", Long.toString(length));

                OutputStream outputStream = ctx.getOutputStream();


                if(partInfoList != null) {
                    return xdi.readMultipart(token, blobInfo, outputStream);
                } else {
                    return xdi.readToOutputStream(token, blobInfo, outputStream, 0, blobInfo.getBlobDescriptor().byteCount);
                }
            });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }
}
