package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;

public class S3UserMetadataUtility {
    private static final String s3UserMetadataPrefix = "x-amz-meta-";

    public static Map<String, String> requestUserMetadata(Request request) {
        HashMap<String, String> umd = new HashMap<>();
        Enumeration<String> headerNames = request.getHeaderNames();
        while(headerNames.hasMoreElements()) {
            String headerName = headerNames.nextElement();
            if(headerName.startsWith(s3UserMetadataPrefix)) {
                umd.put(headerName, request.getHeader(headerName));
            }
        }
        return umd;
    }

    public static Map<String, String> extractUserMetadata(Map<String, String> fdsMetadata) {
        HashMap<String, String> umd = new HashMap<>();
        for(Map.Entry<String, String> fdsMetadataEntry : fdsMetadata.entrySet()) {
            if(fdsMetadataEntry.getKey().startsWith(s3UserMetadataPrefix))
                umd.put(fdsMetadataEntry.getKey(), fdsMetadataEntry.getValue());
        }
        return umd;
    }
}
