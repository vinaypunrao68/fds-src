package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;
import org.junit.Test;

import java.util.NoSuchElementException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class CircularIteratorTest {
    @Test
    public void testCircularIterator() {
        CircularIterator<String> iterator = new CircularIterator<String>(() -> Lists.newArrayList("hello", "world").iterator());
        assertTrue(iterator.hasNext());
        assertEquals("hello", iterator.next());
        assertTrue(iterator.hasNext());
        assertEquals("world", iterator.next());
        assertTrue(iterator.hasNext());
        assertEquals("hello", iterator.next());
        assertTrue(iterator.hasNext());
        assertEquals("world", iterator.next());
        assertTrue(iterator.hasNext());
        assertEquals("hello", iterator.next());
        assertTrue(iterator.hasNext());
        assertEquals("world", iterator.next());
    }

    @Test(expected = NoSuchElementException.class)
    public void testEmtpyIterator() {
        new CircularIterator<String>(() -> Lists.<String>newArrayList().iterator());
    }
}
