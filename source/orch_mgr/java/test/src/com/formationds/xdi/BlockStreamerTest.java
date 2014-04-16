package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;
import junit.framework.TestCase;
import org.apache.commons.io.IOUtils;

import java.io.IOException;
import java.io.InputStream;
import java.util.Iterator;

public class BlockStreamerTest extends TestCase {
    public void testDechunk() throws IOException {
        Iterator<byte[]> sources = Lists.newArrayList("when ", "in ", "the ", "course")
                .stream()
                .map(s -> s.getBytes())
                .iterator();

        InputStream in = new BlockStreamer(sources);
        assertEquals("when in the course", IOUtils.toString(in));
    }
}
