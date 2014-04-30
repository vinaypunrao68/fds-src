package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.ArrayList;
import java.util.Map;
import java.util.Optional;

public class GetObject implements RequestHandler {
    private Xdi xdi;

    public GetObject(Xdi xdi) {
        this.xdi = xdi;
    }

    private class Range
    {
        public Optional<Long> rangeMin;
        public Optional<Long> rangeMax;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        ArrayList<Range> ranges = parseRanges(request.getHeader("Range"));
        return null;
    }

    private ArrayList<Range> parseRanges(String rangeDefinition) {
        ArrayList<Range> range = new ArrayList<Range>();
        if(rangeDefinition == null)
            return range;

        String[] rangeSpecs = rangeDefinition.split(",");
        for(String rangeSpec : rangeSpecs){

        }
        return null;
    }
}
