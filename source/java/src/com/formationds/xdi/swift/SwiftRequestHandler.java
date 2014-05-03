package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.UsageException;
import org.eclipse.jetty.server.Request;

interface SwiftRequestHandler extends RequestHandler {
    public default ResponseFormat obtainFormat(Request request) throws UsageException {
        try {
            return ResponseFormat.valueOf(optionalString(request, "format", ResponseFormat.plain.toString()));
        } catch (Exception e) {
            return ResponseFormat.plain;
        }
    }
}
