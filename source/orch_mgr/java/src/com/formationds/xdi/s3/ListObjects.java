package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.shim.BlobDescriptor;
import org.eclipse.jetty.server.Request;

import java.util.List;
import java.util.Map;

public class ListObjects implements RequestHandler {
    private Xdi xdi;

    public ListObjects(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        List<BlobDescriptor> contents = xdi.volumeContents(S3Endpoint.FDS_S3, requiredString(routeParameters, "bucket"), Integer.MAX_VALUE, 0);
        return null;
    }

    private final static String OBJECT_FORMAT =
            "    <Contents>\n" +
                    "        <Key>%s</Key>\n" +
                    "        <!-- LastModified>2009-10-12T17:50:30.000Z</LastModified -->\n" +
                    "        <!-- ETag>&quot;fba9dede5f27731c9771645a39863328&quot;</ETag -->\n" +
                    "        <Size>%d</Size>\n" +
                    "        <!-- StorageClass>STANDARD</StorageClass -->\n" +
                    "        <!-- Owner>\n" +
                    "            <ID>75aa57f09aa0c8caeab4f8c24e99d10f8e7faeebf76c078efc7c6caea54ba06a</ID>\n" +
                    "            <DisplayName>mtd@amazon.com</DisplayName>\n" +
                    "        </Owner -->\n" +
                    "    </Contents>\n";

    private static final String RESPONSE_FORMAT =
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                    "<ListBucketResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n" +
                    "    <Name>bucket</Name>\n" +
                    "    <Prefix/>\n" +
                    "    <Marker/>\n" +
                    "    <!-- MaxKeys>1000</MaxKeys -->\n" +
                    "    <IsTruncated>false</IsTruncated>\n" +
                    "    <Contents>\n" +
                    "%s    \n" +
                    "    </Contents>\n" +
                    "</ListBucketResult>    \n";
    /*



     */
}
