package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StaticFileHandler;
import com.formationds.web.toolkit.StreamResource;
import com.formationds.xdi.Xdi;
import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.Multimap;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;

import java.io.InputStream;
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
        String domain = routeParameters.get("account");
        String volume = routeParameters.get("container");
        String object = routeParameters.get("object");

        ArrayList<Range> ranges = parseRanges(request.getHeader("Range"));
        BlobDescriptor stat = xdi.statBlob(domain, volume, object);

        InputStream objStream = null;
        if(ranges.size() == 0) {
            objStream = xdi.readStream(domain, volume, object);
        } else if(ranges.size() == 1) {
            objStream = readStreamForRange(xdi, domain, volume, object, ranges.get(0), stat.byteCount);
        } else {
            // FIXME: support multiple ranges
            throw new Exception("multiple ranges are not supported");
        }

        // FIXME: add Accept-Ranges header
        // FIXME: add actual value for Last-Modified header

        String contentType = stat.getMetadata().getOrDefault("Content-Type", StaticFileHandler.getMimeType(object));
        return new StreamResource(objStream, contentType) {
            @Override
            public Multimap<String, String> extraHeaders() {
                LinkedListMultimap<String, String> headers = LinkedListMultimap.create();
                headers.put("ETag", Hex.encodeHexString(stat.getDigest()));
                headers.put("Last-Modified", Long.toString(DateTime.now().getMillis()));
                headers.put("Date", DateTime.now().toString());
                return headers;
            };
        };
    }

    private InputStream readStreamForRange(Xdi xdi, String domain, String volume, String object, Range range, long blobLength) throws Exception {
        // NB: this case is inconsistently specified in the openstack API docs
        if(range.rangeMin.isPresent() && range.rangeMax.isPresent())
            return xdi.readStream(domain, volume, object, range.rangeMin.get(), 1 + range.rangeMax.get() - range.rangeMin.get());

        if(range.rangeMin.isPresent())
            return xdi.readStream(domain, volume, object, range.rangeMin.get(), blobLength - range.rangeMin.get());

        if(range.rangeMax.isPresent())
            return xdi.readStream(domain, volume, object, blobLength - range.rangeMax.get(), range.rangeMax.get() );

        throw new Exception("Invalid range specified");
    }

    private ArrayList<Range> parseRanges(String rangeDefinition) {
        rangeDefinition = rangeDefinition.replaceFirst("bytes=", "").trim();

        ArrayList<Range> ranges = new ArrayList<Range>();
        if(rangeDefinition == null)
            return ranges;

        String[] rangeSpecs = rangeDefinition.split(",");
        for(String rangeSpec : rangeSpecs){
            Range range = new Range();

            String[] rangeElements = rangeSpec.split("-");

            if(!rangeElements[0].isEmpty())
                range.rangeMin = Optional.of(Long.parseLong(rangeElements[0]));
            if(!rangeElements[1].isEmpty())
                range.rangeMax = Optional.of(Long.parseLong(rangeElements[1]));

            ranges.add(range);
        }
        return ranges;
    }
}
