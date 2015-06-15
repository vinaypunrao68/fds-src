package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Optional;

public class CaptureSegmentTemplateElement implements PathTemplateElement {

    private String name;
    private PathTemplateElement element;

    public CaptureSegmentTemplateElement(String name, PathTemplateElement element) {

        this.name = name;
        this.element = element;
    }

    @Override
    public boolean matchPathSegment(String segment) {
        return element.matchPathSegment(segment);
    }

    @Override
    public Optional<KeyValuePair<String, String>> captureParameter(String segment) {
        return Optional.of(new KeyValuePair<String, String>(name, segment));
    }
}
