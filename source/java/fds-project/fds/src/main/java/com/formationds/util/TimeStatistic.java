package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Deque;
import java.util.LinkedList;
import java.util.concurrent.TimeUnit;

public class TimeStatistic {
    private Deque<Key> deque;
    private final long maxAgeMs;


    public TimeStatistic(int maxAge, TimeUnit unit) {
        this.maxAgeMs = unit.toMillis(maxAge);
        deque = new LinkedList<>();
    }

    public long currentTimeMs() {
        return System.currentTimeMillis();
    }

    public synchronized void add(double v) {
        evictOldies();
        deque.addFirst(new Key(currentTimeMs(), v));
    }

    private void evictOldies() {
        while (deque.size() != 0) {
            Key key = deque.peekLast();
            long age = currentTimeMs() - key.timestamp;
            if (age > maxAgeMs) {
                deque.removeLast();
            } else {
                break;
            }
        }
    }

    public synchronized int n() {
        evictOldies();
        return deque.size();
    }

    private class Key {
        long timestamp;
        double value;

        private Key(long timestamp, double value) {
            this.timestamp = timestamp;
            this.value = value;
        }
    }


}
