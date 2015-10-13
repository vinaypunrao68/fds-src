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
import com.formationds.xdi.BlobInfo;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.io.BlobSpecifier;
import org.apache.commons.codec.binary.Hex;

import javax.xml.stream.XMLEventReader;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.events.StartElement;
import javax.xml.stream.events.XMLEvent;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.DigestOutputStream;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public class MultiPartUploadComplete implements SyncRequestHandler {
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
    public Resource handle(HttpContext ctx) throws Exception {
        String bucket = ctx.getRouteParameter("bucket");
        String objectName = ctx.getRouteParameter("object");
        Optional<String> uploadId = ctx.getQueryParameters().get("uploadId").stream().findAny();
        if(!uploadId.isPresent())
            throw new Exception("upload id is not present");

        BlobSpecifier specifier = new BlobSpecifier(S3Endpoint.FDS_S3, bucket, S3Namespace.user().blobName(objectName));
        MultipartUpload multipart = xdi.multipart(token, specifier, uploadId.get());

        //TODO: verify etag map
        Map<Integer, String> etagMap = deserializeEtagMap(ctx.getInputStream());

        BlobDescriptor descriptor = multipart.complete().get();
        String etag = descriptor.getMetadata().get("etag");

        XmlElement response = new XmlElement("CompleteMultipartUploadResult")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/")
                .withValueElt("Location", "http://localhost:8000/" + bucket + "/" + objectName) // TODO: get real URI
                .withValueElt("Bucket", bucket)
                .withValueElt("Key", objectName)
                .withValueElt("ETag", "\"" + etag + "\"");

        return new XmlResource(response.documentString());
    }
}
