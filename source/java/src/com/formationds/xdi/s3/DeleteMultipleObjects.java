package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class DeleteMultipleObjects implements RequestHandler {
    private Xdi xdi;
    private String bucketName;

    public DeleteMultipleObjects(Xdi xdi, String bucketName) {
        this.xdi = xdi;
        this.bucketName = bucketName;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        StringBuffer result = new StringBuffer();
        result.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                "<DeleteResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">");

        new DeleteObjectsFrame(request.getInputStream(), s -> {
            try {
                xdi.deleteBlob(S3Endpoint.FDS_S3, bucketName, s);
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
