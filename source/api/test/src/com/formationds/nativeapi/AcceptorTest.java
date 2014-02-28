package com.formationds.nativeapi;

import junit.framework.TestCase;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class AcceptorTest extends TestCase {
    public void testBasic() throws Exception {
        Acceptor<Integer> acceptor = new Acceptor<>();
        Thread thread = new Thread(() -> {
            sleep(2000);
            acceptor.accept(42);
        });
        assertFalse(acceptor.isDone());
        thread.start();
        assertEquals(42, (int)acceptor.get());
        assertEquals(42, (int)acceptor.get());
        assertTrue(acceptor.isDone());
    }

    private void sleep(int ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}
