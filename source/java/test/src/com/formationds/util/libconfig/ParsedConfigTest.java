package com.formationds.util.libconfig;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class ParsedConfigTest {
    @Test
    public void testParse() throws Exception {
        ParsedConfig adapter = new ParsedConfig(input);
        assertEquals("bar", adapter.lookup("fds.foo").stringValue());
        assertEquals(42, adapter.lookup("fds.hello").intValue());
        assertTrue(adapter.lookup("fds.panda").booleanValue());
    }

    @Test(expected = RuntimeException.class)
    public void testError() throws Exception {
        new ParsedConfig("poop");
    }

    private final static String input =
            "fds: {\n" +
            "    foo=\"bar\"\n" +
            "    hello=42\n" +
            "    panda=true;\n" +
            "    /* This is a comment */\n" +
            "    om: {\n" +
            "       more=\"yes\"\n" +
            "    }\n" +
            "}";
}

