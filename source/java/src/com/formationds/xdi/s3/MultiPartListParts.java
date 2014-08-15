package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class MultiPartListParts implements RequestHandler {
    private Xdi xdi;

    public MultiPartListParts(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucket = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
        String uploadId = request.getParameter("uploadId");

        MultiPartOperations mops = new MultiPartOperations(xdi, uploadId);
        Integer maxParts = getIntegerFromRequest(request, "max-parts");
        Integer partNumberMarker = getIntegerFromRequest(request, "part-number-marker");

        List<PartInfo> allParts = mops.getParts();
        Stream<PartInfo> partInfoStream = allParts.stream();
        if(partNumberMarker != null)
            partInfoStream = partInfoStream.filter(pi -> partNumberMarker < pi.partNumber );
        if(maxParts != null)
            partInfoStream = partInfoStream.limit(maxParts);

        List<PartInfo> filteredList = partInfoStream.collect(Collectors.toList());
        Optional<PartInfo> nextPart = allParts.stream()
                .filter(p -> filteredList.size() == 0 && p.partNumber > filteredList.get(filteredList.size() - 1).partNumber)
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

        if(partNumberMarker != null)
            elt = elt.withValueElt("PartNumberMarker", partNumberMarker.toString());
        if(maxParts != null)
            elt = elt.withValueElt("MaxParts", maxParts.toString());
        if(nextPart.isPresent())
            elt = elt.withValueElt("NextPartNumberMarker", Integer.toString(nextPart.get().partNumber));

        for(PartInfo pi : filteredList) {
            Map<String, String> md = pi.descriptor.getMetadata();
            BlobDescriptor bd = xdi.statBlob(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, pi.descriptor.getName());

            elt = elt.withNest("Part", sub -> sub
                    .withValueElt("PartNumber", Integer.toString(pi.partNumber))
                    .withValueElt("ETag", md.get("etag"))
                    .withValueElt("LastModified", md.get(Xdi.LAST_MODIFIED))
                    .withValueElt("Size", Long.toString(pi.descriptor.getByteCount()))
            );
        }

        return new XmlResource(elt.documentString(), 200);
    }

    public Integer getIntegerFromRequest(Request r, String key) {
        String value = r.getParameter(key);
        if(value == null)
            return null;

        return Integer.parseInt(value);
    }
}
