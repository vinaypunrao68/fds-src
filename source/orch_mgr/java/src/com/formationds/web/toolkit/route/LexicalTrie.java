package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.base.Joiner;

import java.util.HashMap;
import java.util.Map;

public abstract class LexicalTrie<T> {
    protected Map<Character, LexicalTrie<T>> children;

    protected LexicalTrie() {
        children = new HashMap<>();
    }

    public QueryResult<T> find(String s) {
        return find(s, new HashMap<String, String>());
    }

    protected  abstract QueryResult<T> find(String s, Map<String, String> captured);

    public abstract LexicalTrie<T> put(String s, T t);

    public static <T> LexicalTrie<T> newTrie() {
        return new Root<T>();
    }
}

class Root<T> extends LexicalTrie<T> {
    private Capture<T> capture;

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
            LexicalTrie<T> child = children.getOrDefault(s.charAt(0), new Char<T>(s.charAt(0)));
            children.put(s.charAt(0), child.put(s.substring(1), t));
        }
        return this;
    }

    @Override
    public String toString() {
        return "(root " + Joiner.on(", ").join(children.values()) + ")";
    }
}

class Capture<T> extends LexicalTrie<T> {
    private String binding;
    private T t;

    @Override
    public QueryResult<T> find(String s, Map<String, String> captured) {
        StringBuffer value = new StringBuffer();
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
    public Capture<T> put(String s, T t) {
        StringBuffer bindingName = new StringBuffer();
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
            LexicalTrie<T> child = children.getOrDefault(s.charAt(0), new Char<T>(s.charAt(0)));
            children.put(s.charAt(0), child.put(s.substring(1), t));
        } else {
            this.t = t;
        }
        return this;
    }

    @Override
    public String toString() {
        return "(capture@" + binding + " " + Joiner.on(", ").join(children.values()) + ")";
    }
}

class Char<T> extends LexicalTrie<T> {
    private char c;
    private T t;
    private Capture<T> capture;

    Char(char c) {
        this.c = c;
    }

    @Override
    public QueryResult<T> find(String s, Map<String, String> captured) {
        if (s.charAt(0) != c) {
            return new QueryResult<>();
        }

        if (s.length() == 1) {
            if (t != null) {
                return new QueryResult<T>(captured, t);
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
    public Char<T> put(String s, T t) {
        if (s.length() == 0) {
            this.t = t;
        } else {
            char next = s.charAt(0);
            if (next == ':') {
                capture = new Capture<T>().put(s.substring(1), t);
            } else {
                LexicalTrie<T> child = children.getOrDefault(next, new Char(next));
                children.put(next, child.put(s.substring(1), t));
            }
        }

        return this;
    }

    @Override
    public String toString() {
        return "(" + c + " " + Joiner.on(", ").join(children.values()) + displayCapture() + ")";
    }

    private String displayCapture() {
        return capture == null ? "" : capture.toString();
    }
}