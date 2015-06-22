package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class StaticFileHandlerTest {

    @Test
    public void testMimeType() {
        String type = StaticFileHandler.getMimeType("Makefile");
        assertEquals("application/octet-stream", type);
    }
}
