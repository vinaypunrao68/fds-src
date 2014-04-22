package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class CountsTest {

    @Test
    public void testBasic() {
        Counts counts = new Counts();
        counts.increment("foo");
        counts.increment("foo");
        counts.increment("bar");
        Counts consumed = counts.consume();
        assertEquals(2, (int)consumed.get("foo"));
        assertEquals(1, (int)consumed.get("bar"));
    }

    @Test
    public void testMerge() {
        Counts left = new Counts();
        left.increment("foo");
        left.increment("foo");

        Counts right = new Counts();
        right.increment("foo");
        right.increment("bar");

        Counts merged = left.merge(right).consume();
        assertEquals(3, (int)merged.get("foo"));
        assertEquals(1, (int)merged.get("bar"));
    }
}
