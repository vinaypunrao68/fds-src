package com.formationds.commons.libconfig;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Maps;

import java.util.HashMap;
import java.util.Map;

public class Namespace implements Node {
    private String name;
    private Map<String, Node> children;

    Namespace(String name) {
        this.name = name;
        children = new HashMap<>();
    }

    @Override
    public String getName() {
        return name;
    }

    public Namespace addChild(Node node) {
        children.put(node.getName(), node);
        return this;
    }

    public Map<String, Node> children() {
        return Maps.newHashMap(children);
    }
}

