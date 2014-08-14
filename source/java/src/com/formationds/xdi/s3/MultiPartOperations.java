package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.xdi.Xdi;
import org.apache.thrift.TException;

import java.io.ByteArrayInputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

class MultiPartOperations {
    private Xdi xdi;
    private String txId;

    MultiPartOperations(Xdi xdi, String txId) {
        this.xdi = xdi;
        this.txId = txId;
    }

    public String getPartName(int partNumber) {
        return txId + "-" + String.format("%05d", partNumber);
    }

    public static List<String> getMultipartUploadsInProgress(Xdi xdi) throws TException, ApiException {
        Pattern p = Pattern.compile("^multipart-(.+)$");
        ArrayList<String> uploadIds = new ArrayList<>();

        // TODO: read this in chunks
        List<BlobDescriptor> blobDescriptors = xdi.volumeContents(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, Integer.MAX_VALUE, 0);
        for(BlobDescriptor bd : blobDescriptors) {
            Matcher match = p.matcher(bd.getName());
            if(match.matches()) {
                uploadIds.add(match.group(1));
            }
        }

        return uploadIds;
    }

    // TODO: there is obviously a race here when the number of blobs in the system bucket changes
    //       given the current API it may not be feasible to prevent it
    public List<PartInfo> getParts() throws TException, ApiException {
        Pattern p = Pattern.compile("^" + Pattern.quote(txId) + "-(\\d{5})$");
        ArrayList<PartInfo> partDescriptors = new ArrayList<>();

        // TODO: read this in chunks
        List<BlobDescriptor> blobDescriptors = xdi.volumeContents(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, Integer.MAX_VALUE, 0);
        for(BlobDescriptor bd : blobDescriptors) {
            Matcher match = p.matcher(bd.getName());
            if(match.matches()) {
                PartInfo pi = new PartInfo();
                pi.descriptor = bd;
                pi.partNumber = Integer.parseInt(match.group(1));
                partDescriptors.add(pi);
            }
        }

        partDescriptors.sort((p1, p2) -> Integer.compare(p1.partNumber, p2.partNumber));

        return partDescriptors;
    }
}

class PartInfo {
    public BlobDescriptor descriptor;
    public int partNumber;
}