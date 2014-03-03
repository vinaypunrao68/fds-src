package com.formationds.web;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Multimap;

import javax.servlet.http.Cookie;
import java.io.IOException;
import java.io.OutputStream;

public interface Resource {
    public default Cookie[] cookies() {
        return new Cookie[0];
    }

    public int getHttpStatus();

    public String getContentType();

    public Multimap<String, String> extraHeaders();

    public void render(OutputStream outputStream) throws IOException;
}
