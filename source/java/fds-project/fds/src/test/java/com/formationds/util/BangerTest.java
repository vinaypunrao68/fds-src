package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.math3.stat.descriptive.SummaryStatistics;
import org.junit.Test;

import java.util.Map;

public class BangerTest {
    @Test
    public void testBang() {
        Banger banger = new Banger(10, 10, 1, () -> () -> {
            try {
                Thread.sleep(1);
            } catch (InterruptedException e) {
            }
        });
        Map<Integer, SummaryStatistics> results = banger.bang();
        results.keySet()
                .stream()
                .sorted()
                .forEach(k -> {
                    SummaryStatistics stats = results.get(k);
                    System.out.println(k + ": mean=" + stats.getMean() + ", stdev=" + stats.getStandardDeviation());
                });
    }
}
