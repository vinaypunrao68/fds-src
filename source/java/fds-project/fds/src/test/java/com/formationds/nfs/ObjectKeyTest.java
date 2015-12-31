package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.junit.Test;

import static org.junit.Assert.*;

public class ObjectKeyTest {
    @Test
    public void testEqualHashCodeAndCompare() throws Exception {
        ObjectKey left = new ObjectKey("foo", "bar", "panda", new ObjectOffset(42));
        ObjectKey right = new ObjectKey("foo", "bar", "panda", new ObjectOffset(42));
        ObjectKey center = new ObjectKey("foo", "bar", "panda", new ObjectOffset(43));
        assertTrue(left.equals(right));
        assertTrue(left.hashCode() == right.hashCode());
        assertFalse(left.equals(center));
        assertFalse(left.hashCode() == center.hashCode());
        assertCommonPrefix(left.bytes(), right.bytes(), 11);
        assertTrue(center.compareTo(left) > 0);
    }

    public void assertCommonPrefix(byte[] left, byte[] right, int prefixLength) {
        for (int i = 0; i < prefixLength; i++) {
            if (left[i] != right[i]) {
                fail("Arrays don't share a common prefix");
            }
        }
    }
}