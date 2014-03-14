package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import junit.framework.Assert;
import org.junit.Test;

public class MutableTest {
    @Test
    public void testAwaitValue() throws Exception {
        Mutable<Integer> mutable = new Mutable<>();
        new Thread(() -> {sleepALittle(); mutable.set(42);}).start();
        int result = mutable.awaitValue();
        Assert.assertEquals(42, result);
    }

    private void sleepALittle() {
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}
