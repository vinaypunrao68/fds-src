package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.BlobDescriptor;
import com.formationds.spike.later.HttpContext;
import com.formationds.xdi.security.XdiAcl;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

public class S3MetadataUtility {
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

    public static void updateMetadata(Map<String, String> metadata, Optional<BlobDescriptor> extantDescriptor) {
        if(extantDescriptor.isPresent())
            updateMetadata(extantDescriptor.get().getMetadata(), metadata);
        else
            updateMetadata(new HashMap<>(), metadata);
    }


    public static void updateMetadata(Map<String, String> metadata, Map<String, String> priorMetadata) {
        if(priorMetadata.containsKey(XdiAcl.X_AMZ_ACL) && !metadata.containsKey(XdiAcl.X_AMZ_ACL)) {
            metadata.put(XdiAcl.X_AMZ_ACL, priorMetadata.get(XdiAcl.X_AMZ_ACL));
        }
    }
}
