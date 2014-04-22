package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class SingleTest {
    @Test
    public void testBlock() {
        Single<Integer> single = new Single<Integer>();
        new Thread(() -> single.set(42)).start();
        assertEquals(42, (int) single.get());
        single.set(71);
        assertEquals(71, (int) single.get());
    }
}
