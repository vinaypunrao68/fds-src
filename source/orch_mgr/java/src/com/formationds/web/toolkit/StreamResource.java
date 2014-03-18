package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.io.IOUtils;

import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class StreamResource implements Resource {
    private InputStream inputStream;
    private String contentType;

    public StreamResource(InputStream inputStream, String contentType) {
        this.inputStream = inputStream;
        this.contentType = contentType;
    }

    @Override
    public int getHttpStatus() {
        return HttpServletResponse.SC_OK;
    }

    @Override
    public String getContentType() {
        return contentType;
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        IOUtils.copy(inputStream, outputStream);
        outputStream.close();
    }
}
