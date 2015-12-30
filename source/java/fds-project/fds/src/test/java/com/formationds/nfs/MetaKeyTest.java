package com.formationds.nfs;

import org.junit.Test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class MetaKeyTest {
    @Test
    public void testBeginsWith() throws Exception {
        MetaKey prefix = new MetaKey("foo", "bar", "hello/");
        MetaKey child = new MetaKey("foo", "bar", "hello/panda");
        MetaKey nonChild = new MetaKey("foo", "bar", "boo");
        MetaKey otherKey = new MetaKey("foo", "ploop", "boo");
        assertTrue(child.beginsWith(prefix));
        assertFalse(nonChild.beginsWith(prefix));
        assertFalse(otherKey.beginsWith(prefix));
    }
}