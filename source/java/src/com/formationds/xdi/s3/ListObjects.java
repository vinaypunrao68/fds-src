package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class ListObjects implements RequestHandler {
    private Xdi xdi;

    public ListObjects(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucket = requiredString(routeParameters, "bucket");
        List<BlobDescriptor> contents = xdi.volumeContents(S3Endpoint.FDS_S3, bucket, Integer.MAX_VALUE, 0);

        List<String> objects = contents.stream()
                .map(c -> String.format(OBJECT_FORMAT, c.getName(), c.getByteCount()))
                .collect(Collectors.toList());

        String response = String.format(RESPONSE_FORMAT, bucket, String.join("", objects));
        return new XmlResource(response);
    }

    private final static String OBJECT_FORMAT =
                    "<Contents><Key>%s</Key><Size>%d</Size></Contents>";

    private static final String RESPONSE_FORMAT =
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                    "<ListBucketResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" +
                    "<Name>%s</Name><Prefix></Prefix><Marker></Marker><MaxKeys>1000</MaxKeys><IsTruncated>false</IsTruncated>" +
                    "%s" +
                    "</ListBucketResult>";
}
