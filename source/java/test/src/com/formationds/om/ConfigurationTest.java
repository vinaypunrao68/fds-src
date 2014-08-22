package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.Configuration;

import static org.junit.Assert.assertEquals;

public class ConfigurationTest {
    //@Test
    public void testParseArguments() throws Exception {
        assertEquals("/foo", new Configuration("foo", new String[] {"--fds-root=/foo"}).getFdsRoot());
        assertEquals("/fds", new Configuration("foo", new String[0]).getFdsRoot());
    }
}
