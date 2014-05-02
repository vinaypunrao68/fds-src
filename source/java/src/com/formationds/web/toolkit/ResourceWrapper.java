package com.formationds.web.toolkit;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Multimap;

import javax.servlet.http.Cookie;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;

public class ResourceWrapper implements Resource {
    private Resource _resource;
    private Multimap<String, String> _headers;
    private ArrayList<String> _cookies;

    public ResourceWrapper(Resource res) {
        _resource = res;
    }

    @Override
    public String getContentType() {
        return _resource.getContentType();
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        _resource.render(outputStream);
    }

    @Override
    public Multimap<String, String> extraHeaders() {
        Multimap<String, String> subHeaders = _resource.extraHeaders();
        subHeaders.putAll(_headers);

        return subHeaders;
    }

    @Override
    public Cookie[] cookies() {
        Cookie[] baseCookies = _resource.cookies();
        if(_cookies.size() == 0)
            return baseCookies;

        Cookie[] result = new Cookie[baseCookies.length + _cookies.size()];
        System.arraycopy(baseCookies, 0, result, 0, baseCookies.length);
        System.arraycopy(_cookies.toArray(), 0, result, baseCookies.length, _cookies.size());
        return result;
    }

    @Override
    public ResourceWrapper withHeader(String key, String value) {
        _headers.put(key, value);
        return this;
    }
}
