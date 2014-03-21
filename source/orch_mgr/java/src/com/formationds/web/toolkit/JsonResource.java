package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.json.JSONArray;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;

public class JsonResource implements Resource {
    private Object o;
    private int httpResponseCode;

    public JsonResource(JSONObject object) {
        init(object, HttpServletResponse.SC_OK);
    }

    public JsonResource(JSONArray array) {
        init(array, HttpServletResponse.SC_OK);
    }

    public JsonResource(JSONObject o, int httpResponseCode) {
        init(o, httpResponseCode);
    }

    private void init(Object o, int httpResponseCode) {
        this.o = o;
        this.httpResponseCode = httpResponseCode;
    }

    @Override
    public String getContentType() {
        return "application/json";
    }

    @Override
    public int getHttpStatus() {
        return httpResponseCode;
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        OutputStreamWriter writer = new OutputStreamWriter(outputStream);
        writer.write(o.toString());
        writer.write('\n');
        writer.flush();
    }
}
