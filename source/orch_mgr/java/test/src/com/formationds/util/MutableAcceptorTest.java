package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.base.Joiner;
import com.google.common.collect.Lists;
import junit.framework.TestCase;

public class MutableAcceptorTest extends TestCase {
    public void testAccept() {
        MutableAcceptor<Integer> acceptor = new MutableAcceptor<>();
        assertFalse(acceptor.isDone());
        Lists.newArrayList(1, 2, 3).forEach(i -> acceptor.accept(i));
        acceptor.finish();
        assertTrue(acceptor.isDone());
        assertEquals("1, 2, 3", Joiner.on(", ").join(acceptor));
    }
}
