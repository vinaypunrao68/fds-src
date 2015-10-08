package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.ApiException;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;

public class DeleteMultipleObjects implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteMultipleObjects(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        String bucketName = ctx.getRouteParameter("bucket");
        StringBuffer result = new StringBuffer();
        result.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                "<DeleteResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">");

        new DeleteObjectsFrame(ctx.getInputStream(), s -> {
            try {
                xdi.deleteBlob(token, S3Endpoint.FDS_S3, bucketName, S3Namespace.user().blobName(s)).get();
                result.append("<Deleted><Key>");
                result.append(s);
                result.append("</Key></Deleted>");
            } catch (ApiException exception) {
                result.append(S3FailureHandler.makeS3Failure(s, exception).asSnippet());
            } catch (Exception e) {
                result.append(new S3Failure(S3Failure.ErrorCode.InternalError, e.getMessage(), s));
            }
        }).parse();

        result.append("</DeleteResult>\n");
        return new XmlResource(result.toString());
    }
}
