package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.TimeStatistic;

import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

public class BucketStats {
    private ConcurrentHashMap<String, TimeStatistic> map;
    private int maxAge;
    private TimeUnit timeUnit;

    public BucketStats(int maxAge, TimeUnit timeUnit) {
        this.maxAge = maxAge;
        this.timeUnit = timeUnit;
        map = new ConcurrentHashMap<>();
    }


    public synchronized void increment(String s) {
        map.compute(s, (k, v) -> {
            if (v == null) {
                v = new TimeStatistic(maxAge, timeUnit);
            }

            v.add(1);
            return v;
        });
    }

    public Set<String> keys() {
        return map.keySet();
    }

    public TimeStatistic get(String key) {
        return map.getOrDefault(key, new TimeStatistic(maxAge, timeUnit));
    }
}
