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

    public QueryResult<T> find(String s) {
        return find(s, new HashMap<>());
    }

    public final LexicalTrie<T> put(String s, T t) {
        return put(new StringView(s), t)    ;
    }

    public final QueryResult<T> find(String s, Map<String, String> captured) {
        return find(new StringView(s), captured);
    }

    protected abstract QueryResult<T> find(StringView s, Map<String, String> captured);

    protected abstract LexicalTrie<T> put(StringView s, T t);


    public static <T> LexicalTrie<T> newTrie() {
        return new Root<>();
    }

    protected String displayValueMaybe(T t) {
        if (t != null) {
            return "#" + t;
        }

        return "";
    }
}

