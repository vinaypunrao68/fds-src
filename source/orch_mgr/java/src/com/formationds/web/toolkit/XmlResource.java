package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class XmlResource extends TextResource {
    public XmlResource(String text) {
        super(text);
    }

    @Override
    public String getContentType() {
        return "text/xml; charset=UTF-8";
    }
}
