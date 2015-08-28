package com.formationds.commons.libconfig;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Assert;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class ParsedConfigTest {
    @Test
    public void testParse() throws Exception {
        ParsedConfig adapter = new ParsedConfig(input);
        assertEquals("bar", adapter.lookup("fds.foo").stringValue());
        assertEquals("bar", adapter.defaultString("fds.foo", "panda"));
        assertEquals(42, adapter.lookup("fds.hello").intValue());
        assertTrue(adapter.lookup("fds.panda").booleanValue());
    }

    @Test
    public void defaultValue() throws Exception {
        ParsedConfig adapter = new ParsedConfig(input);
        assertEquals(3, adapter.defaultInt("fds.poop", 3));
        assertEquals(42, adapter.defaultInt("fds.hello", 101));
    }

    @Test
    public void testEmptyGroup() throws Exception {

        ParsedConfig config = new ParsedConfig( input );

        final Assignment pandaAssignment = config.lookup( "fds.panda" );
        Assert.assertTrue( pandaAssignment.booleanValue() );

        final Assignment omAssignment = config.lookup( "fds.emptyGroup" );
        assertTrue(omAssignment.stringValue().isEmpty());

        final Assignment omSnmp = config.lookup( "fds.om.snmp" );
        assertTrue(omSnmp.stringValue().isEmpty());

    }

    @Test(expected = RuntimeException.class)
    public void testError() throws Exception {
        new ParsedConfig("poop");
    }

    private final static String input =
            "fds: {\n" +
            "    foo=\"bar\"\n" +
            "    hello=42\n" +
            "    /* This is a comment */\n" +
            "    om: {\n" +
            "       more=\"yes\"\n" +
            "    }\n" +
            "    emptyGroup: {\n" +
            "    }\n" +
            "    panda=true;\n" +
            "}";
}

