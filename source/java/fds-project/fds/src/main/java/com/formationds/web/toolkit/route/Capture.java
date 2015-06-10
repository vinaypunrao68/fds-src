package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.base.Joiner;

import java.util.Map;

public class Capture<T> extends LexicalTrie<T> {
    private String binding;
    private T t;

    @Override
    public QueryResult<T> find(StringView s, Map<String, String> captured) {
        StringBuilder value = new StringBuilder();
        int i = 0;
        for (; i < s.length() && s.charAt(i) != '/'; i++) {
            value.append(s.charAt(i));
        }
        captured.put(binding, value.toString());
        s = s.substring(i);

        if (s.length() > 0) {
            if (children.containsKey(s.charAt(0))) {
                return children.get(s.charAt(0)).find(s, captured);
            }
        } else if (t != null) {
            return new QueryResult<>(captured, t);
        }

        return new QueryResult<>();
    }


    @Override
    public Capture<T> put(StringView s, T t) {
        StringBuilder bindingName = new StringBuilder();
        int i = 0;
        for (; i < s.length() && s.charAt(i) != '/'; i++) {
            if (s.charAt(i) == ':') {
                throw new RuntimeException("Malformed pattern");
            }
            bindingName.append(s.charAt(i));
        }
        binding = bindingName.toString();
        s = s.substring(i);
        if (s.length() > 0) {
            LexicalTrie<T> child = children.getOrDefault(s.charAt(0), new Char<>(s.charAt(0)));
            children.put(s.charAt(0), child.put(s.substring(1), t));
        } else {
            this.t = t;
        }
        return this;
    }

    @Override
    public String toString() {
        return "(capture@" + binding + displayValueMaybe(t) + Joiner.on(" , ").join(children.values()) + ")";
    }
}
