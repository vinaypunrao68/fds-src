package com.formationds.web;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.Resource;
import org.dom4j.Element;
import org.dom4j.io.OutputFormat;
import org.dom4j.io.XMLWriter;

import java.io.IOException;
import java.io.OutputStream;

public class Dom4jResource implements Resource {
    private Element root;

    public Dom4jResource(Element root) {
        this.root = root;
    }

    @Override
    public String getContentType() {
        return "application/xml; charset=utf-8";
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        new XMLWriter(outputStream, OutputFormat.createPrettyPrint()).write(root);
    }
}
