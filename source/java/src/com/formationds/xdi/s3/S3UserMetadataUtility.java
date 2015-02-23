package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.spike.later.HttpContext;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

public class S3UserMetadataUtility {
    private static final String s3UserMetadataPrefix = "x-amz-meta-";

    public static Map<String, String> requestUserMetadata(HttpContext context) {
        HashMap<String, String> umd = new HashMap<>();
        Collection<String> headerNames = context.getRequestHeaderNames();
        for (String headerName : headerNames) {
            if(headerName.startsWith(s3UserMetadataPrefix)) {
                umd.put(headerName, context.getRequestHeader(headerName));
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
