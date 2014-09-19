package com.formationds.util.libconfig;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.antlr.runtime.ANTLRInputStream;
import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.tree.CommonTree;

import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

public class ParsedConfig {
    private Map<String, Node> map;

    public ParsedConfig(String input) {
        this(new ANTLRStringStream(input));
    }

    public ParsedConfig(InputStream inputStream) throws IOException {
        this(new ANTLRInputStream(inputStream));
    }

    public ParsedConfig(CharStream charStream) {
        LibConfigParser parser = new LibConfigParser(new CommonTokenStream(new LibConfigLexer(charStream)));
        LibConfigParser.namespace_return v = null;
        try {
            v = parser.namespace();
        } catch (RecognitionException e) {
            throw new RuntimeException("Parse error");
        }
        map = new HashMap<>();
        Node node = doParse((CommonTree) v.getTree());
        map.put(node.getName(), node);

    }

    private Node doParse(CommonTree commonTree) {
        switch (commonTree.getType()) {
            case LibConfigParser.ID:
                Namespace namespace = new Namespace(commonTree.getText());
                commonTree.getChildren().stream()
                        .map(t -> (CommonTree) t)
                        .forEach(c -> namespace.addChild(doParse(c)));
                return namespace;

            case LibConfigParser.EQUALS:
                CommonTree[] children = commonTree.getChildren().stream()
                        .map(t -> (CommonTree) t)
                        .toArray(i -> new CommonTree[i]);
                String name = children[0].getText();
                Object value = parseValue(children[1]);
                return new Assignment(name, value);

            default:
                throw new RuntimeException("Unrecognized token type, " + commonTree.getType());
        }
    }

    private Object parseValue(CommonTree child) {
        switch (child.getType()) {
            case LibConfigParser.INT:
                return Integer.parseInt(child.getText());

            case LibConfigParser.FLOAT:
                return Float.parseFloat(child.getText());

            case LibConfigParser.STRING:
                return child.getText().replaceAll("\"", "");

            case LibConfigParser.BOOLEAN:
                return Boolean.parseBoolean(child.getText());

            default:
                throw new RuntimeException("Unrecognized token type, " + child.getType());
        }
    }

    public Assignment lookup(String path) {
        String[] parts = path.split("\\.");
        Map<String, Node> current = map;

        for (int i = 0; i < parts.length; i++) {
            String part = parts[i];
            if (!current.containsKey(part)) {
                throw new RuntimeException("key not found: '" + path + "'");
            }

            Node node = current.get(part);

            if (i == parts.length - 1) {
                if (!(node instanceof Assignment)) {
                    throw new RuntimeException("key not found: '" + path + "'");
                }
                return (Assignment) node;
            } else {
                if (!(node instanceof Namespace)) {
                    throw new RuntimeException("key not found: '" + path + "'");
                }
                current = ((Namespace) node).children();
            }
        }

        throw new RuntimeException("key not found: '" + path + "'");
    }

    public int defaultInt(String s, int defaultValue) {
        try {
            return lookup(s).intValue();
        } catch (Exception e) {
            return defaultValue;
        }
    }

    public boolean defaultBoolean(String key, boolean def) {
        try {
            return lookup(key).booleanValue();
        } catch(Exception e) {
            return def;
        }
    }

    public String defaultString(String s, String defaultValue) {
        try {
            return lookup(s).stringValue();
        } catch (Exception e) {
            return defaultValue;
        }
    }
}
