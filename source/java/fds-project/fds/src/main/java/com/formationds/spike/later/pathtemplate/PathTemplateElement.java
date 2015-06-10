package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Optional;

public interface PathTemplateElement {
    public boolean matchPathSegment(String segment);
    public Optional<KeyValuePair<String, String>> captureParameter(String segment);
}
