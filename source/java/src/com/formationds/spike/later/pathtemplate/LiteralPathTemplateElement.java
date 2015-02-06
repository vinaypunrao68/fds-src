package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Optional;

public class LiteralPathTemplateElement implements PathTemplateElement {
    private String literalString;
    private boolean caseInsensitive;

    public LiteralPathTemplateElement(String literalString, boolean caseInsensitve) {
        this.literalString = literalString;
        this.caseInsensitive = caseInsensitve;
    }

    @Override
    public boolean matchPathSegment(String segment) {
        if(caseInsensitive) {
            return literalString.equalsIgnoreCase(segment);
        } else {
            return literalString.equals(segment);
        }
    }

    @Override
    public Optional<KeyValuePair<String, String>> captureParameter(String segment) {
        return Optional.empty();
    }

    public static LiteralPathTemplateElement empty = new LiteralPathTemplateElement("", false);
}
