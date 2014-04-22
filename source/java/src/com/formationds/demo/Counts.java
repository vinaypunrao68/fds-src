package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class Counts {
    private Map<String, Integer> map;

    public Counts() {
        this(new HashMap<>());
    }

    private Counts(Map<String, Integer> map) {
        this.map = map;
    }

    public synchronized void increment(String s) {
        map.compute(s, (k, v) -> {
            if (v == null) {
                return 1;
            } else {
                return v + 1;
            }
        });
    }

    public Counts consume() {
        Map<String, Integer> oldMap = map;
        map = new HashMap<>();
        return new Counts(oldMap);
    }

    public Set<String> keys() {
        return map.keySet();
    }

    public int get(String key) {
        return map.get(key);
    }

    public Counts merge(Counts right) {
        Map<String, Integer> result = new HashMap<>();
        mergeAll(result, map);
        mergeAll(result, right.map);
        return new Counts(result);
    }

    private void mergeAll(Map<String, Integer> to, Map<String, Integer> from) {
        for (Map.Entry<String, Integer> entry : from.entrySet()) {
            to.compute(entry.getKey(), (k, v) -> {
                if (v == null)
                    v = 0;

                return v + entry.getValue();
            });
        }
    }
}
