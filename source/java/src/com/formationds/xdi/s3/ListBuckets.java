package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.services.s3.internal.ServiceUtils;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class ListBuckets implements RequestHandler {
    private Xdi xdi;

    public ListBuckets(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        List<VolumeDescriptor> volumeDescriptors = xdi.listVolumes(S3Endpoint.FDS_S3);
        String buckets = String.join("", volumeDescriptors.stream()
                .map(v -> displayBucket(v))
                .collect(Collectors.toList()));

        String result = String.format(RESULT_FORMAT, S3Endpoint.FDS_S3, buckets);
        return new XmlResource(result);
    }

    private String displayBucket(VolumeDescriptor v) {
        DateTime dateTime = new DateTime(v.getDateCreated());
        return String.format(BUCKET_FORMAT, v.getName(), ServiceUtils.formatIso8601Date(dateTime.toDate()));
    }

    private static final String BUCKET_FORMAT =
            "    <Bucket><Name>%s</Name><CreationDate>%s</CreationDate></Bucket>";

    private static final String RESULT_FORMAT =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                    "<ListAllMyBucketsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01\">\n" +
                    "  <Owner>\n" +
                    "    <ID>bcaf1ffd86f461ca5fb16fd081034f</ID>\n" +
                    "    <DisplayName>%s</DisplayName>\n" +
                    "  </Owner>\n" +
                    "  <Buckets>\n" +
                    "%s\n" +
                    "  </Buckets>\n" +
                    "</ListAllMyBucketsResult>\n";
}