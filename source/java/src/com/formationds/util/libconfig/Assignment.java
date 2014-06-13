package com.formationds.util.libconfig;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Assignment implements Node {
    private String name;
    private Object value;

    Assignment(String name, Object value) {
        this.name = name;
        this.value = value;
    }

    public String getName() {
        return name;
    }

    public int intValue() {
        return (Integer) value;
    }

    public String stringValue() {
        return (String) value;
    }

    public boolean booleanValue() {
        return (Boolean) value;
    }
}
