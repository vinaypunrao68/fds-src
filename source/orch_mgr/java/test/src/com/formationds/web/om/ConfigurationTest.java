package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ConfigurationTest {
    @Test
    public void testParseArguments() throws Exception {
        assertEquals("/foo", new Configuration(new String[] {"--fds-root=/foo"}).getFdsRoot());
        assertEquals("/fds", new Configuration(new String[0]).getFdsRoot());
    }
}
