package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.HashMap;
import java.util.Map;

public abstract class LexicalTrie<T> {
    protected Map<Character, LexicalTrie<T>> children;

    protected LexicalTrie() {
        children = new HashMap<>();
    }

    public QueryResult find(String s) {
        return null;
    }
    public abstract LexicalTrie<T> put(String s, T t);
}

class Root<T> extends LexicalTrie<T> {
    @Override
    public LexicalTrie<T> put(String s, T t) {
        return null;
    }
}

class Capture<T> extends LexicalTrie<T> {
    private String binding;
    private LexicalTrie<T> child;
    private T t;

    @Override
    public Capture<T> put(String s, T t) {
        StringBuffer bindingName = new StringBuffer();
        for (int i = 0; i < s.length(); i++) {
            if (s.charAt(i) == ':') {
                throw new RuntimeException("Malformed pattern");
            }
            if (s.charAt(i) == '/') {
                binding = bindingName.toString();
                child = new Character<T>().put(s.substring(i), t);
            }
            bindingName.append(s.charAt(i));
        }
        return this;
    }
}

class Character<T> extends LexicalTrie<T> {

    @Override
    public LexicalTrie<T> put(String s, T t) {

        return null;
    }
}