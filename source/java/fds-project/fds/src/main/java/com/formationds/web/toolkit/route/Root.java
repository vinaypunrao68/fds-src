package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.base.Joiner;

import java.util.HashMap;
import java.util.Map;

public class Root<T> extends LexicalTrie<T> {
    private Capture<T> capture;

    @Override
    public QueryResult<T> find(StringView s, Map<String, String> captured) {
        if (capture != null) {
            return capture.find(s, new HashMap<>());
        } else if (children.containsKey(s.charAt(0))) {
            return children.get(s.charAt(0)).find(s, new HashMap<>());

        } else {
            return new QueryResult<>();
        }
    }

    @Override
    public Root<T> put(StringView s, T t) {
        if (s.length() == 0) {
            return this;
        } else if (s.charAt(0) == ':') {
            if (s.length() == 1) {
                throw new RuntimeException("Malformed pattern");
            }
            capture = new Capture<T>().put(s.substring(1), t);
        } else {
            LexicalTrie<T> child = children.getOrDefault(s.charAt(0), new Char<>(s.charAt(0)));
            children.put(s.charAt(0), child.put(s.substring(1), t));
        }
        return this;
    }

    @Override
    public String toString() {
        return "(root " + Joiner.on(", ").join(children.values()) + ")";
    }
}
