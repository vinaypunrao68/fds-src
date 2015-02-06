package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Optional;

public class AnyPathTemplateElement implements PathTemplateElement {
    @Override
    public boolean matchPathSegment(String segment) {
        return true;
    }

    @Override
    public Optional<KeyValuePair<String, String>> captureParameter(String segment) {
        return Optional.empty();
    }
}
