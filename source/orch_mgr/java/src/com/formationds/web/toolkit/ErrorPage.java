package com.formationds.web.toolkit;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class ErrorPage implements Resource {
    private String message;
    private Throwable throwable;

    public ErrorPage(String message, Throwable throwable) {
        this.message = message;
        this.throwable = throwable;
    }

    @Override
    public int getHttpStatus() {
        return 503;
    }

    @Override
    public String getContentType() {
        return "text/plain; charset=UTF-8";
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        PrintWriter writer = new PrintWriter(new OutputStreamWriter(outputStream));
        writer.println("Internal server error - 503");
        writer.println(message);
        throwable.printStackTrace(writer);
        writer.flush();
    }
}
