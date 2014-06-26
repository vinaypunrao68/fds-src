package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class SizeTest {
    @Test
    public void testSize() {
        Size size = Size.size(2);
        assertEquals(SizeUnit.B, size.getSizeUnit());
        assertEquals(2, (int)size.getCount());

        size = Size.size(1243);
        assertEquals(SizeUnit.KB, size.getSizeUnit());
        assertEquals(1.213, size.getCount(), .001);

        size = Size.size(2222444);
        assertEquals(SizeUnit.MB, size.getSizeUnit());
        assertEquals(2.119, size.getCount(), .001);
    }
}
