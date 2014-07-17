package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.base.Joiner;

import java.util.Map;

public class Char<T> extends LexicalTrie<T> {
    private char c;
    private T t;
    private Capture<T> capture;

    Char(char c) {
        this.c = c;
    }

    @Override
    public QueryResult<T> find(StringView s, Map<String, String> captured) {
        if (s.charAt(0) != c) {
            return new QueryResult<>();
        }

        if (s.length() == 1) {
            if (t != null) {
                return new QueryResult<>(captured, t);
            } else {
                return new QueryResult<>();
            }
        } else {
            s = s.substring(1);
            if (children.containsKey(s.charAt(0))) {
                return children.get(s.charAt(0)).find(s, captured);
            } else if (capture != null && s.length() > 0) {
                return capture.find(s.substring(0), captured);
            }
            return new QueryResult<>();
        }
    }

    @Override
    public Char<T> put(StringView s, T t) {
        if (s.length() == 0) {
            this.t = t;
        } else {
            char next = s.charAt(0);
            if (next == ':') {
                capture = capture == null? new Capture<T>().put(s.substring(1), t) : capture;
                capture.put(s.substring(1), t);
            } else {
                LexicalTrie<T> child = children.getOrDefault(next, new Char<>(next));
                children.put(next, child.put(s.substring(1), t));
            }
        }

        return this;
    }

    @Override
    public String toString() {
        return "(" + c + displayValueMaybe(t) + " " + Joiner.on(", ").join(children.values()) + displayCapture() + ")";
    }

    private String displayCapture() {
        return capture == null ? "" : capture.toString();
    }
}
