package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.base.Joiner;

import java.util.HashMap;
import java.util.Map;

public abstract class LexicalTrie<T> {
    protected Map<Char, LexicalTrie<T>> children;

    protected LexicalTrie() {
        children = new HashMap<>();
    }

    public abstract QueryResult<T> find(String s, Map<String, String> captured);

    public abstract LexicalTrie<T> put(String s, T t);
}

class Root<T> extends LexicalTrie<T> {
    private Map<Character, Char<T>> children;
    private Capture<T> capture;

    Root() {
        children = new HashMap<>();
    }

    public QueryResult<T> find(String s) {
        return find(s, new HashMap<String, String>());
    }

    @Override
    public QueryResult<T> find(String s, Map<String, String> captured) {
        if (capture != null) {
            return capture.find(s, new HashMap<String, String>());
        } else if (children.containsKey(s.charAt(0))) {
            return children.get(s.charAt(0)).find(s, new HashMap<String, String>());

        } else {
            return new QueryResult<T>();
        }
    }

    @Override
    public Root<T> put(String s, T t) {
        if (s.length() == 0) {
            return this;
        } else if (s.charAt(0) == ':') {
            if (s.length() == 1) {
                throw new RuntimeException("Malformed pattern");
            }
            capture = new Capture<T>().put(s.substring(1), t);
        } else {
            Char<T> child = children.getOrDefault(s.charAt(0), new Char<T>(s.charAt(0)));
            children.put(s.charAt(0), child.put(s.substring(1), t));
        }
        return this;
    }

    @Override
    public String toString() {
        return "(root: " + Joiner.on(", ").join(children.values()) + ")";
    }
}

class Capture<T> extends LexicalTrie<T> {
    private String binding;
    private LexicalTrie<T> child;
    private T t;

    @Override
    public QueryResult<T> find(String s, Map<String, String> captured) {
        StringBuffer value = new StringBuffer();
        for (int i = 0; i < s.length() || s.charAt(i) != '/'; i++) {
            value.append(s.charAt(i));
        }

        return null;

    }


    @Override
    public Capture<T> put(String s, T t) {
        StringBuffer bindingName = new StringBuffer();
        int i = 0;
        for (; i < s.length() || s.charAt(i) != '/'; i++) {
            if (s.charAt(i) == ':') {
                throw new RuntimeException("Malformed pattern");
            }
            bindingName.append(s.charAt(i));
        }
        binding = bindingName.toString();
        if (s.length() > 0) {
            child = new Char<T>(s.charAt(i)).put(s.substring(i), t);
        } else {
            this.t = t;
        }
        return this;
    }
}

class Char<T> extends LexicalTrie<T> {
    private char c;
    private T t;
    private Map<Character, Char<T>> children;
    private Capture<T> capture;

    Char(char c) {
        this.c = c;
        children = new HashMap<>();
    }

    @Override
    public QueryResult<T> find(String s, Map<String, String> captured) {
        if (s.charAt(0) == c) {
            if (s.length() == 1) {
                if (t == null) {
                    return new QueryResult<>();
                } else {
                    return new QueryResult<T>(captured, t);
                }
            } else {
                if (children.containsKey(s.charAt(1))) {
                    return children.get(s.charAt(1)).find(s.substring(1), captured);
                }
            }
        }
        return new QueryResult<>();
    }

    @Override
    public Char<T> put(String s, T t) {
        if (s.length() == 0) {
            this.t = t;
        } else {
            char next = s.charAt(0);
            if (next == ':') {
                capture = new Capture<T>().put(s.substring(1), t);
            } else {
                Char<T> child = children.getOrDefault(next, new Char(next));
                children.put(next, child.put(s.substring(1), t));
            }
        }

        return this;
    }

    @Override
    public String toString() {
        return "(" + c + ": " + Joiner.on(", ").join(children.values()) + ")";
    }
}