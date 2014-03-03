package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Multimap;

import javax.servlet.http.Cookie;
import java.io.IOException;
import java.io.OutputStream;

public interface Resource {
    public int getHttpStatus();

    public String getContentType();

    public default Multimap<String, String> extraHeaders() {
        return ArrayListMultimap.create();
    }

    public default Cookie[] cookies() {
        return new Cookie[0];
    }

    public void render(OutputStream outputStream) throws IOException;
}
