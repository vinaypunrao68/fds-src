package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StaticFileHandler;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.Multimap;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;

import java.util.HashMap;
import java.util.Map;

public class CreateObject implements RequestHandler {
    private Xdi xdi;

    public CreateObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String domain = routeParameters.get("account");
        String volume = routeParameters.get("container");
        String object = routeParameters.get("object");

        String contentType = request.getHeader("Content-Type");

        if (contentType == null) {
            contentType = StaticFileHandler.getMimeType(object);
        }

        Map<String, String> metadata = new HashMap<>();
        metadata.put("Content-Type", contentType);

        byte[] digest = xdi.writeStream(domain, volume, object, request.getInputStream(), metadata);

        final String finalContentType = contentType;
        TextResource resource = new TextResource(201, "") {
            @Override
            public Multimap<String, String> extraHeaders() {
                LinkedListMultimap<String, String> headers = LinkedListMultimap.create();
                headers.put("ETag", Hex.encodeHexString(digest));
                headers.put("Content-Type", finalContentType);
                headers.put("Date", DateTime.now().toString());
                return headers;
            }
        };
        return resource;
    }
}
