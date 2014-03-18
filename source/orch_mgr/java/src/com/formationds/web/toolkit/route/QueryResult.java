package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Map;

public class QueryResult<T> {
    private Map<String, String> matches;
    private T value;

    public boolean found() {
        return false;
    }

    public Map<String, String> getMatches() {
        return matches;
    }

    public T getValue() {
        return value;
    }
}
