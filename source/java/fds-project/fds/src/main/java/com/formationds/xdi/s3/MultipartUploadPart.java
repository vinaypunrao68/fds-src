package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.io.BlobSpecifier;
import org.apache.commons.codec.binary.Hex;
import org.apache.thrift.TException;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiFunction;

public class MultipartUploadPart implements BiFunction<HttpContext, AuthenticationToken, CompletableFuture<Void>> {
    private Xdi xdi;

    public MultipartUploadPart(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public CompletableFuture<Void> apply(HttpContext context, AuthenticationToken authenticationToken) {
        String bucket = context.getRouteParameter("bucket");
        String object = context.getRouteParameter("object");
        Optional<String> uploadid = context.getQueryParameters().get("uploadId").stream().findAny();
        if(!uploadid.isPresent())
            throw new IllegalArgumentException("upload id is not specified in request");

        Optional<Integer> partNumber = context.getQueryParameters().get("partNumber").stream().findAny().map(m -> Integer.parseInt(m));
        if(!partNumber.isPresent())
            throw new IllegalArgumentException("part number is not specified");

        Optional<Long> contentLength = S3Endpoint.getContentLength(context);
        if(!contentLength.isPresent())
            throw new IllegalArgumentException("content length is not specified");


        BlobSpecifier specifier = new BlobSpecifier(S3Endpoint.FDS_S3, bucket, S3Namespace.user().blobName(object));
        try {
            DateTime now = new DateTime(DateTimeZone.UTC);
            return xdi.multipart(authenticationToken, specifier, uploadid.get()).uploadPart(context.getInputStream(), partNumber.get(), contentLength.get(), now)
                    .thenAccept(digest -> {
                        context.setResponseStatus(200);
                        context.addResponseHeader("ETag", '"' + Hex.encodeHexString(digest) + '"');
                    });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }
}
