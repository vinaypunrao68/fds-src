package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;

import javax.xml.stream.XMLEventReader;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.events.StartElement;
import javax.xml.stream.events.XMLEvent;
import java.io.InputStream;
import java.io.SequenceInputStream;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MultiPartUploadComplete implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public MultiPartUploadComplete(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
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
            if(e.isEndElement() && e.asEndElement().getName().equals(elt.getName()))
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
        String uploadId = request.getQueryParameters().getString("uploadId");
        MultiPartOperations mops = new MultiPartOperations(xdi, uploadId, token);

        //TODO: verify etag map
        Map<Integer,String> etagMap = deserializeEtagMap(request.getInputStream());

        // TODO: do this read asynchronously
        List<PartInfo> partInfoList = mops.getParts();

        InputStream is = null;
        for(PartInfo bd : partInfoList) {
            InputStream str = xdi.readStream(token, S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, bd.descriptor.getName());
            if(is == null)
                is = str;
            else
                is = new SequenceInputStream(is, str);
        }

        Map<String, String> metadata = new HashMap<>();
        // TODO: do this write asynchronously
        byte[] digest = xdi.writeStream(token, S3Endpoint.FDS_S3, bucket, objectName, is, metadata);

        for(PartInfo bd : partInfoList)
            xdi.deleteBlob(token, S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, bd.descriptor.getName());

        XmlElement response = new XmlElement("CompleteMultipartUploadResult")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/")
                .withValueElt("Location", "http://" + request.getServerName() + ":8000/" + bucket + "/" + objectName) // TODO: get real URI
                .withValueElt("Bucket", bucket)
                .withValueElt("Key", objectName)
                .withValueElt("ETag", "\"" + Hex.encodeHexString(digest) + "\"");

        return new XmlResource(response.documentString());
    }
}
