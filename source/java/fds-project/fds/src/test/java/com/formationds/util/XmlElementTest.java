package com.formationds.util;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class XmlElementTest {
    @Test
    public void testRenderWithIndent() {
        String xml = new XmlElement("Foo")
                .withValueElt("Hello", "world")
                .documentString();

        String expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                "<Foo>\n" +
                "  <Hello>world</Hello>\n" +
                "</Foo>\n";

        assertEquals(expected, xml);
    }

    @Test
    public void testRenderMinified() {
        String result = new XmlElement("Foo")
                .withValueElt("Hello", "world")
                .minifiedDocumentString();

        String expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                "<Foo><Hello>world</Hello></Foo>\n";

        assertEquals(expected, result);
    }

}