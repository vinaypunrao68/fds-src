package com.formationds.web.toolkit;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.Multimap;

import javax.servlet.http.Cookie;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;

public class ResourceWrapper implements Resource {
    private Resource resource;
    private Multimap<String, String> headers;
    private ArrayList<String> cookies;
    private String contentType;

    public ResourceWrapper(Resource res) {
        resource = res;
        headers = LinkedListMultimap.create();
        cookies = new ArrayList<>();
    }

    @Override
    public String getContentType() {
        return contentType != null ? contentType : resource.getContentType();
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        resource.render(outputStream);
    }

    @Override
    public Multimap<String, String> extraHeaders() {
        Multimap<String, String> subHeaders = resource.extraHeaders();
        subHeaders.putAll(headers);

        return subHeaders;
    }

    @Override
    public Cookie[] cookies() {
        Cookie[] baseCookies = resource.cookies();
        if(cookies.size() == 0)
            return baseCookies;

        Cookie[] result = new Cookie[baseCookies.length + cookies.size()];
        System.arraycopy(baseCookies, 0, result, 0, baseCookies.length);
        System.arraycopy(cookies.toArray(), 0, result, baseCookies.length, cookies.size());
        return result;
    }

    @Override
    public int getHttpStatus() {
        return resource.getHttpStatus();
    }

    @Override
    public Resource withHeader(String key, String value) {
        headers.put(key, value);
        return this;
    }

    @Override
    public Resource withContentType(String contentType) {
        this.contentType = contentType;
        return this;
    }
}
