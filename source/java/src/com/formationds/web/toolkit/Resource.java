package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.spike.later.HttpContext;
import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Multimap;
import io.undertow.server.handlers.CookieImpl;

import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Arrays;

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

    public default void renderTo(HttpContext ctx) {
        Arrays.stream(cookies()).forEach(c -> ctx.addCookie(c.getName(), toUndertowCookie(c)));

        ctx.setResponseContentType(getContentType());
        ctx.setResponseStatus(getHttpStatus());

        Multimap<String, String> extraHeaders = extraHeaders();
        for (String headerName : extraHeaders.keySet()) {
            for (String value : extraHeaders.get(headerName)) {
                ctx.addResponseHeader(headerName, value);
            }
        }

        ClosingInterceptor outputStream = new ClosingInterceptor(ctx.getOutputStream());
        try {
            render(outputStream);
            outputStream.flush();
            outputStream.doCloseForReal();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public default io.undertow.server.handlers.Cookie toUndertowCookie(Cookie c) {
        return new CookieImpl(c.getName(), c.getValue()).setDomain(c.getDomain()).setMaxAge(c.getMaxAge());
    }
}
