package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public enum SizeUnit {
    B(1l),
    KB(1024l),
    MB((long)Math.pow(1024l, 2l)),
    GB((long)Math.pow(1024l, 3l)),
    TB((long)Math.pow(1024l, 4l));

    private long multiplier;

    SizeUnit(long multiplier) {
        this.multiplier = multiplier;
    }

    public long totalBytes(double count) {
        return (long) (count * multiplier);
    }
}
