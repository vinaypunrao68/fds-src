package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StaticFileHandler;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;

import java.util.HashMap;
import java.util.Map;

public class CreateObject implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public CreateObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String domain = requiredString(routeParameters, "account");
        String volume = requiredString(routeParameters, "container");
        String object = requiredString(routeParameters, "object");

        String contentType = request.getHeader("Content-Type");

        if (contentType == null) {
            contentType = StaticFileHandler.getMimeType(object);
        }

        // TODO: factor this into common XDI stuff
        Map<String, String> metadata = new HashMap<String, String>();
        metadata.put("Content-Type", contentType);
        metadata.put("Last-Modified", SwiftUtility.formatRfc1123Date(DateTime.now()));

        byte[] digest = xdi.put(token, domain, volume, object, request.getInputStream(), metadata).get().digest;


        // TODO: add real transactionId instead of call to swiftResource here
        return SwiftUtility.swiftResource(new TextResource(201, ""))
                .withHeader("ETag", Hex.encodeHexString(digest))
                .withHeader("Content-Type", contentType);
    }
}
