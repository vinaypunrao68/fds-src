package com.formationds.web.toolkit.route;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class StringView {
    private String s;
    private int offset;

    public StringView(String s) {
        this.s = s;
        offset = 0;
    }

    private StringView(String s, int offset) {
        this.s = s;
        this.offset = offset;
    }

    public Character charAt(int i) {
        return s.charAt(i + offset);
    }

    public int length() {
        return s.length() - offset;
    }

    public StringView substring(int i) {
        return new StringView(s, offset + i);
    }

    @Override
    public String toString() {
        return s.substring(offset);
    }
}
