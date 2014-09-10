package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Multimap;

import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.OutputStream;

public interface Resource {
    public String getContentType();

    public void render(OutputStream outputStream) throws IOException;

    public default Multimap<String, String> extraHeaders() {
        return ArrayListMultimap.create();
    }

    public default Cookie[] cookies() {
        return new Cookie[0];
    }

    public default int getHttpStatus() {
        return HttpServletResponse.SC_OK;
    }

    public default Resource withHeader(String key, String value) { return new ResourceWrapper(this).withHeader(key, value);  }

    public default Resource withContentType(String contentType) { return new ResourceWrapper(this).withContentType(contentType); }
}
