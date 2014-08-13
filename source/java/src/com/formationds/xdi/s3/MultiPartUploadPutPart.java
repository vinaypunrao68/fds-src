package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class MultiPartUploadPutPart implements RequestHandler {
    private Xdi xdi;

    public MultiPartUploadPutPart(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucket = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
        String txid = request.getParameter("uploadId");
        String partNumberStr = request.getParameter("partNumber");
        int partNumber = Integer.parseInt(partNumberStr);

        if(partNumber < 0 || partNumber > 10000)
            throw new Exception("invalid part number");

        HashMap<String, String> metadata = new HashMap<>();

        byte[] digest = xdi.writeStream(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME,
                txid + "-" + String.format("%05d", partNumber), request.getInputStream(), metadata);

        return new TextResource("")
                .withHeader("Etag", "\"" + Hex.encodeHexString(digest) + "\"" );
    }
}
