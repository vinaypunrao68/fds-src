package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.XmlResource;

import javax.servlet.http.HttpServletResponse;
import java.util.UUID;

public class S3Failure extends XmlResource {
    static enum ErrorCode {
        AccessDenied(HttpServletResponse.SC_FORBIDDEN),
        NoSuchKey(HttpServletResponse.SC_NOT_FOUND),
        InvalidRequest(HttpServletResponse.SC_BAD_REQUEST),
        InternalError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR),
        ServiceUnavailable(HttpServletResponse.SC_SERVICE_UNAVAILABLE),
        BucketAlreadyExists(HttpServletResponse.SC_CONFLICT),
        BucketNotEmpty(HttpServletResponse.SC_CONFLICT);

        private int httpStatus;

        ErrorCode(int httpStatus) {
            this.httpStatus = httpStatus;
        }

        public int getHttpStatus() {
            return httpStatus;
        }
    }

    public S3Failure(ErrorCode errorCode, String message, String resource) {
        super(String.format(FORMAT, errorCode.toString(), message, resource, UUID.randomUUID().toString()), errorCode.getHttpStatus());
    }

    private static String FORMAT = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<Error>" +
            "<Code>%s</Code>" +
            "<Message>%s</Message>" +
            "<Resource>%s</Resource>" +
            "<RequestId>%s</RequestId>" +
            "</Error>\n";
}
