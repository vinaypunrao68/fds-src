package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Multimap;

import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.OutputStream;

public class HttpRedirect implements Resource {
    private String location;

    public HttpRedirect(String location) {
        this.location = location;
    }

    @Override
    public int getHttpStatus() {
        return HttpServletResponse.SC_FOUND;
    }

    @Override
    public String getContentType() {
        return "text/plain";
    }

    @Override
    public Multimap<String, String> extraHeaders() {
        ArrayListMultimap<String, String> map = ArrayListMultimap.create();
        map.put("Location", location);
        return map;
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {

    }
}
