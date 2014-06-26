package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.web.toolkit.RequestHandler;

import java.util.function.Supplier;

public class S3FailureHandler implements Supplier<RequestHandler> {
    private Supplier<RequestHandler> supplier;

    public S3FailureHandler(Supplier<RequestHandler> supplier) {
        this.supplier = supplier;
    }

    @Override
    public RequestHandler get() {
        return (request, routeParameters) -> {
            String requestedResource = request.getRequestURI();

            try {
                return supplier.get().handle(request, routeParameters);
            } catch (ApiException e) {
                return makeS3Failure(requestedResource, e);
            }
        };
    }

    public static S3Failure makeS3Failure(String requestedResource, ApiException e) {
        switch (e.getErrorCode()) {
            case MISSING_RESOURCE:
                return new S3Failure(S3Failure.ErrorCode.NoSuchKey, "No such resource", requestedResource);

            case BAD_REQUEST:
                return new S3Failure(S3Failure.ErrorCode.InvalidRequest, "Invalid request", requestedResource);

            case INTERNAL_SERVER_ERROR:
                return new S3Failure(S3Failure.ErrorCode.InternalError, e.getMessage(), requestedResource);

            case SERVICE_NOT_READY:
                return new S3Failure(S3Failure.ErrorCode.ServiceUnavailable, "The service is unavailable (no storage nodes?)", requestedResource);

            case RESOURCE_ALREADY_EXISTS:
                return new S3Failure(S3Failure.ErrorCode.BucketAlreadyExists, "Bucket already exists", requestedResource);

            case RESOURCE_NOT_EMPTY:
                return new S3Failure(S3Failure.ErrorCode.BucketNotEmpty, "Bucket is not empty", requestedResource);

            default:
                return new S3Failure(S3Failure.ErrorCode.InternalError, e.getMessage(), requestedResource);
        }
    }
}
