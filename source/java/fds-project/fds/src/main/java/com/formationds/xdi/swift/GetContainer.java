package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.PatternSemantics;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.JsonArrayCollector;
import com.formationds.web.W3cXmlResource;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.google.common.base.Joiner;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;
import org.json.JSONArray;
import org.json.JSONObject;
import org.w3c.dom.Element;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class GetContainer  implements SwiftRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public GetContainer(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }


    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String accountName = requiredString(routeParameters, "account");
        String containerName = requiredString(routeParameters, "container");

        int limit = optionalInt(request, "limit", Integer.MAX_VALUE);
        ResponseFormat format = obtainFormat(request);
        List<BlobDescriptor> descriptors = null;
        try {
            descriptors = xdi.volumeContents(token,
                                             accountName,
                                             containerName,
                                             limit,
                                             0,
                                             "",
                                             BlobListOrder.UNSPECIFIED,
                                             false,
                                             PatternSemantics.PCRE,
                                             "").get();
        } catch (TException e) {
            throw new ApiException("Not found", ErrorCode.MISSING_RESOURCE);
        }

        descriptors = new SkipUntil<BlobDescriptor>(request.getParameter("marker"), b -> b.getName())
                .apply(descriptors);

        Resource result;
        switch (format) {
            case xml:
                result = xmlView(containerName, descriptors);
                break;

            case json:
                result = jsonView(descriptors);
                break;

            default:
                result = plainView(descriptors);
                break;
        }

        return SwiftUtility.swiftResource(result);
    }

    private Resource plainView(List<BlobDescriptor> descriptors) {
        String textView = Joiner.on("\n").join(descriptors.stream().map(d -> d.getName()).collect(Collectors.toList()));
        return new TextResource(textView);
    }

    private Resource jsonView(List<BlobDescriptor> descriptors) {
        JSONArray array = descriptors.stream()
                .map( d -> new JSONObject()
                               .put( "hash", digest( d ) )
                               .put( "last_modified", lastModified( d ) )
                               .put( "bytes", d.getByteCount() )
                               .put( "name", d.getName() )
                               .put( "content_type", contentType( d ) ) )
                .collect( new JsonArrayCollector() );
        return new JsonResource(array);
    }

    private String contentType(BlobDescriptor d) {
        return d.getMetadata().getOrDefault("Content-Type", "application/octet-stream");
    }

    private String digest(BlobDescriptor d) {
        return d.getMetadata().getOrDefault("etag", "");
    }

    private String lastModified(BlobDescriptor d) {
        String timestamp = d.getMetadata().get(Xdi.LAST_MODIFIED);
        try {
            return new DateTime(Long.parseLong(timestamp)).toString();
        } catch (Exception e) {
            return DateTime.now().toString();
        }
    }

    protected Resource xmlView(String containerName, List<BlobDescriptor> descriptors) throws Exception {
        DocumentBuilder builder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
        org.w3c.dom.Document document = builder.newDocument();

        Element root = document.createElement( "container" );
        root.setAttribute( "name", containerName );

        descriptors.stream()
                   .forEach( d -> {

                       Element object = document.createElement( "object" );
                       root.appendChild( object );

                       Element name = document.createElement( "name" );
                       name.setTextContent( d.getName() );
                       object.appendChild( name );

                       Element hash = document.createElement( "hash" );
                       hash.setTextContent( digest( d ) );
                       object.appendChild( hash );

                       Element bytes = document.createElement( "bytes" );
                       bytes.setTextContent( Long.toString( d.getByteCount() ) );
                       object.appendChild( bytes );

                       Element ct = document.createElement( "content_type" );
                       ct.setTextContent( contentType( d ) );
                       object.appendChild( ct );

                       Element lastModified = document.createElement( "last_modified" );
                       lastModified.setTextContent( lastModified( d ) );
                       object.appendChild( lastModified );
                   } );

        document.appendChild( root );

        return new W3cXmlResource(document);

    }
}
