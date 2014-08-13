package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import javax.xml.stream.XMLEventReader;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.events.StartElement;
import javax.xml.stream.events.XMLEvent;
import java.io.InputStream;
import java.io.SequenceInputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.stream.Collectors;

public class MultiPartUploadComplete implements RequestHandler {
    private Xdi xdi;

    public MultiPartUploadComplete(Xdi xdi) {
        this.xdi = xdi;
    }

    private Map<Integer, String> deserializeEtagMap(InputStream source) throws Exception{
        Map<Integer, String> result = new HashMap<>();
        XMLInputFactory xif = XMLInputFactory.newFactory();
        XMLEventReader rdr = xif.createXMLEventReader(source);

        int currentPart = -1;
        String eTag = null;
        while(rdr.hasNext()) {
            XMLEvent event = rdr.nextEvent();
            if(event.isStartElement()) {
                StartElement elt = event.asStartElement();
                switch(elt.getName().getLocalPart().toLowerCase()) {
                    case "partnumber":
                        currentPart = Integer.parseInt(getContent(rdr, elt));
                        break;
                    case "etag":
                        eTag = getContent(rdr, elt);
                        break;
                }
            }
            else if(event.isEndElement() && event.asEndElement().getName().getLocalPart().toLowerCase().equals("part")) {
                result.put(currentPart, eTag);
            }
        }

        return result;
    }

    private String getContent(XMLEventReader rdr, StartElement elt) throws Exception {
        String data = "";
        while(rdr.hasNext()) {
            XMLEvent e = rdr.nextEvent();
            if(e.asEndElement().getName().equals(elt.getName()))
                return data;

            if(e.isCharacters())
                data += e.asCharacters().getData();
        }
        return data;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucket = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
        String uploadId = request.getParameter("uploadId");

        // TODO: verify etag map
        // Map<Integer,String> etagMap = deserializeEtagMap(request.getInputStream());

        // TODO: there is obviously a race here when the number of blobs in the system bucket changes
        //       given the current API it may not be feasible to prevent it
        int offset = 0;
        Pattern p = Pattern.compile("^" + Pattern.quote(uploadId) + "-(\\d{5})$");
        ArrayList<BlobDescriptor> partDescriptors = new ArrayList<>();

        // TODO: read this in chunks
        List<BlobDescriptor> blobDescriptors = xdi.volumeContents(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, Integer.MAX_VALUE, 0);
        for(BlobDescriptor bd : blobDescriptors) {
            Matcher match = p.matcher(bd.getName());
            if(match.matches()) {
                partDescriptors.add(bd);
            }
        }

        partDescriptors.sort((b1, b2) -> b1.getName().compareTo(b2.getName()));
        long totalSize = partDescriptors.stream().collect(Collectors.summingLong(i -> i.byteCount));

        InputStream is = null;
        for(BlobDescriptor bd : partDescriptors) {
            InputStream str = xdi.readStream(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, bd.getName());
            if(is == null)
                is = str;
            else
                is = new SequenceInputStream(is, str);
        }

        Map<String, String> metadata = new HashMap<>();
        xdi.writeStream(S3Endpoint.FDS_S3, bucket, objectName, is, metadata);

        for(BlobDescriptor bd : partDescriptors)
            xdi.deleteBlob(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, bd.getName());

        return new TextResource("");
    }
}
