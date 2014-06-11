package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Size {
    private double count;
    private SizeUnit sizeUnit;

    public Size(double count, SizeUnit sizeUnit) {
        this.count = count;
        this.sizeUnit = sizeUnit;
    }

    public double getCount() {
        return count;
    }

    public SizeUnit getSizeUnit() {
        return sizeUnit;
    }

    public long totalBytes() {
        return sizeUnit.totalBytes(count);
    }

    public static Size size(long byteCount) {
        SizeUnit[] values = SizeUnit.values();
        SizeUnit current = SizeUnit.B;
        double dividend = byteCount;

        for (int i = 0; i < values.length; i++) {
            current = values[i];
            dividend = (double) byteCount / (Math.pow(1024, i));
            if (dividend < 1024) {
                break;
            }
        }

        return new Size(dividend, current);
    }
}
