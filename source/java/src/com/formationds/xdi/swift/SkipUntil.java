package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;

public class SkipUntil<T> {
    private String marker;
    private Function<T, String> f;

    public SkipUntil(String marker, Function<T, String> f) {
        this.marker = marker;
        this.f = f;
    }

    public List<T> apply(List<T> tees) {
        if (marker == null) {
            return tees;
        }

        boolean retain = false;
        List<T> filtered = new ArrayList<>();

        for (T tee : tees) {
            if (retain) {
                filtered.add(tee);
            } else {
                if (marker.equals(f.apply(tee))) {
                    retain = true;
                }
            }
        }

        return filtered;
    }
}
