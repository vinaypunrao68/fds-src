package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;

public class JsonResource implements Resource {
    private Object o;

    public JsonResource(JSONObject object) {
        this.o = object;
    }

    public JsonResource(JSONArray array) {
        this.o = array;
    }
    @Override
    public String getContentType() {
        return "application/json";
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        OutputStreamWriter writer = new OutputStreamWriter(outputStream);
        writer.write(o.toString());
        writer.flush();
    }
}
