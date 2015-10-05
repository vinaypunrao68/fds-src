package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.BlobDescriptor;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.io.BlobSpecifier;
import org.apache.commons.codec.binary.Hex;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class MultiPartListParts implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public MultiPartListParts(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext context) throws Exception {
        String bucket = context.getRouteParameter("bucket");
        String objectName = context.getRouteParameter("object");
        Map<String, Collection<String>> qp = context.getQueryParameters();
        String uploadId = qp.get("uploadId").iterator().next();

        BlobSpecifier specifier = new BlobSpecifier(S3Endpoint.FDS_S3, bucket, S3Namespace.user().blobName(objectName));
        MultipartUpload mpu = xdi.multipart(token, specifier, uploadId);
        Integer maxParts = getIntegerFromQueryParameters(qp, "max-parts");
        Integer partNumberMarker = getIntegerFromQueryParameters(qp, "part-number-marker");

        List<MultipartUpload.PartInfo> allParts = mpu.listParts().get();
        Stream<MultipartUpload.PartInfo> partInfoStream = allParts.stream();
        if (partNumberMarker != null)
            partInfoStream = partInfoStream.filter(pi -> partNumberMarker < pi.getPartNumber());
        if (maxParts != null)
            partInfoStream = partInfoStream.limit(maxParts);

        List<MultipartUpload.PartInfo> filteredList = partInfoStream.collect(Collectors.toList());
        Optional<MultipartUpload.PartInfo> nextPart = allParts.stream()
                .filter(p -> filteredList.size() == 0 && p.getPartNumber() > filteredList.get(filteredList.size() - 1).getPartNumber())
                .findFirst();

        XmlElement elt = new XmlElement("ListPartsResult")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/")
                .withValueElt("Bucket", bucket)
                .withValueElt("Key", objectName)
                .withValueElt("UploadId", uploadId)
                .withNest("Initiator", sub -> sub
                        .withValueElt("ID", "NOT_IMPLEMENTED_YET")
                        .withValueElt("DisplayName", "NOT_IMPLEMENTED_YET"))
                .withNest("Owner", sub -> sub
                        .withValueElt("ID", "NOT_IMPLEMENTED_YET")
                        .withValueElt("DisplayName", "NOT_IMPLEMENTED_YET"));

        if (partNumberMarker != null)
            elt = elt.withValueElt("PartNumberMarker", partNumberMarker.toString());
        if (maxParts != null)
            elt = elt.withValueElt("MaxParts", maxParts.toString());
        if (nextPart.isPresent())
            elt = elt.withValueElt("NextPartNumberMarker", Integer.toString(nextPart.get().getPartNumber()));

        for (MultipartUpload.PartInfo pi : filteredList) {
            elt = elt.withNest("Part", sub -> sub
                            .withValueElt("PartNumber", Integer.toString(pi.getPartNumber()))
                            .withValueElt("ETag", Hex.encodeHexString(pi.getEtag()))
                            .withValueElt("LastModified", S3Endpoint.formatAwsDate(pi.getLastModified())))
                            .withValueElt("Size", Long.toString(pi.getLength())
            );
        }

        return new XmlResource(elt.documentString(), 200);
    }

    public Integer getIntegerFromQueryParameters(Map<String, Collection<String>> qp, String key) {
        if (!qp.containsKey(key)) {
            return null;
        }

        String value = qp.get(key).iterator().next();
        return Integer.parseInt(value);
    }
}
