package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.services.cloudfront.model.InvalidArgumentException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

public class XmlElement{
    public static final String XML_PREAMBLE = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    private Map<String, String> attributes;
    private List<XmlElement> children;
    private String name;
    private String value;

    public XmlElement(String name) {
        checkName(name);

        this.name = name;
        attributes = new HashMap<>();
        children = new ArrayList<>();
    }

    public XmlElement(String name, String value) {
        this(name);
        this.value = value;
    }

    private void checkName(String name) {
        if(name.contains("<>\"/\\=!"))
            throw new InvalidArgumentException("invalid element name");
    }

    public XmlElement withAttr(String name, String value) {
        checkName(name);

        attributes.put(name, value);
        return this;
    }

    public XmlElement withElt(XmlElement elt) {
        if(value != null)
            throw new IllegalArgumentException("element cannot have both value and child elements");

        children.add(elt);
        return this;
    }

    public XmlElement withValueElt(String name, String value) {
        return withElt(new XmlElement(name, value));
    }

    public XmlElement withNest(String childName, Consumer<XmlElement> gen) {
        XmlElement newElt = new XmlElement(childName);
        gen.accept(newElt);
        return withElt(newElt);
    }

    private void indent(StringBuilder b, int indentLevel) {
        for(int i = 0; i < indentLevel; i++)
            b.append(" ");
    }

    private void render(StringBuilder b, int indentLevel, boolean minified) {
        if (!minified) {
            indent(b, indentLevel);
        }
        b.append("<");
        b.append(name);
        for (Map.Entry<String, String> entry : attributes.entrySet()) {
            b.append(" ");
            b.append(entry.getKey());
            b.append("=\"");
            b.append(entry.getValue().replace("\"", "\\\""));
            b.append("\"");
        }

        if (attributes.size() > 0)
            b.append(" ");

        boolean multiLine = children.size() > 0 || (value != null && value.contains("\n"));

        b.append(">");

        if (multiLine && !minified)
            b.append("\n");

        for (XmlElement elt : children) {
            elt.render(b, indentLevel + 2, minified);
        }

        if (value != null)
            b.append(value);

        if (multiLine && !minified) {
            //b.append("\n");
            indent(b, indentLevel);
        }
        b.append("</");
        b.append(name);
        b.append(">");

        if (!minified) {
            b.append("\n");
        }
    }

    private void renderDocument(StringBuilder b, boolean minify) {
        b.append(XML_PREAMBLE);
        render(b, 0, minify);
    }

    public String documentString() {
        StringBuilder b = new StringBuilder();
        renderDocument(b, false);
        return b.toString();
    }

    public String minifiedDocumentString() {
        StringBuilder frame = new StringBuilder();
        frame.append(XML_PREAMBLE);
        render(frame, 0, true);
        frame.append("\n");
        return frame.toString();
    }
}
