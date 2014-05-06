package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.base.Joiner;
import com.google.common.collect.Multimap;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ResourceWrapperTest {
    @Test
    public void testBasic() {
        ResourceWrapper resource = new TextResource("poop")
                .withHeader("foo", "bar");
        Multimap<String, String> headers = resource.extraHeaders();
        assertEquals("Content-Length, foo", Joiner.on(", ").join(headers.keySet()));
    }
}
