package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.am.Main;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.shim.VolumeDescriptor;
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
        List<VolumeDescriptor> volumeDescriptors = xdi.listVolumes(Main.FDS_S3);
        String buckets = String.join("", volumeDescriptors.stream()
                .map(v -> displayBucket(v))
                .collect(Collectors.toList()));

        String result = String.format(RESULT_FORMAT, Main.FDS_S3, buckets);
        return new XmlResource(result);
    }

    private String displayBucket(VolumeDescriptor v) {
        return String.format(BUCKET_FORMAT, v.getName(), new DateTime(v.getDateCreated()).toString());
    }

    private static final String BUCKET_FORMAT =
            "    <Bucket>\n" +
                    "      <Name>%s</Name>\n" +
                    "      <CreationDate>%s</CreationDate>\n" +
                    "    </Bucket>\n";

    private static final String RESULT_FORMAT =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                    "<ListAllMyBucketsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01\"\n" +
                    "  <Owner>\n" +
                    "    <ID>bcaf1ffd86f461ca5fb16fd081034f</ID>\n" +
                    "    <DisplayName>%s</DisplayName>\n" +
                    "  </Owner>\n" +
                    "  <Buckets>\n" +
                    "%s" +
                    "  </Buckets>\n" +
                    "</ListAllMyBucketsResult>\n";
}