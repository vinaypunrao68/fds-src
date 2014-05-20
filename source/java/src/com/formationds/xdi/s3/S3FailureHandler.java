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
            try {
                return supplier.get().handle(request, routeParameters);
            } catch (ApiException e) {
                switch (e.getErrorCode()) {
                    case MISSING_RESOURCE:
                        return new S3Failure(S3Failure.ErrorCode.NoSuchKey, "No such resource", request.getRequestURI());

                    case BAD_REQUEST:
                        return new S3Failure(S3Failure.ErrorCode.InvalidRequest, "Invalid request", request.getRequestURI());

                    case INTERNAL_SERVER_ERROR:
                        return new S3Failure(S3Failure.ErrorCode.InternalError, e.getMessage(), request.getRequestURI());

                    case SERVICE_NOT_READY:
                        return new S3Failure(S3Failure.ErrorCode.ServiceUnavailable, "The service is unavailable (no storage nodes?)", request.getRequestURI());

                    case RESOURCE_ALREADY_EXISTS:
                        return new S3Failure(S3Failure.ErrorCode.BucketAlreadyExists, "Bucket already exists", request.getRequestURI());

                    case RESOURCE_NOT_EMPTY:
                        return new S3Failure(S3Failure.ErrorCode.BucketNotEmpty, "Bucket is not empty", request.getRequestURI());

                    default:
                        return new S3Failure(S3Failure.ErrorCode.InternalError, e.getMessage(), request.getRequestURI());
                }
            }
        };
    }
}
