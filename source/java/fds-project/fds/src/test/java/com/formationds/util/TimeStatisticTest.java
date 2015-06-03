package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;

public class TimeStatisticTest {

    @Test
    public void testTimeStatistic() {
        long[] time = new long[1];
        TimeStatistic stat = new TimeStatistic(1, TimeUnit.SECONDS) {
            @Override
            public long currentTimeMs() {
                return time[0];
            }
        };

        stat.add(1d);
        time[0] = 500;
        stat.add(2d);
        assertEquals(2, stat.n());
        time[0] = 1001;
        assertEquals(1, stat.n());

    }
}
