package com.formationds.web.toolkit;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.Multimap;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;


public class TextResource implements Resource {
    private int httpStatus;
    private String text;

    public TextResource(String text) {
        this(200, text);
    }

    public TextResource(int httpStatus, String text) {
        this.httpStatus = httpStatus;
        this.text = text;
    }

    @Override
    public int getHttpStatus() {
        return httpStatus;
    }

    @Override
    public String getContentType() {
        return "text/plain; charset=UTF-8";
    }

    @Override
    public Multimap<String, String> extraHeaders() {
        Multimap<String, String> multimap = LinkedListMultimap.create();
        int length = text.getBytes().length;
        multimap.put("Content-Length", Integer.toString(length));
        return multimap;
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        OutputStreamWriter writer = new OutputStreamWriter(outputStream);
        writer.write(text);
        writer.flush();
    }
}
