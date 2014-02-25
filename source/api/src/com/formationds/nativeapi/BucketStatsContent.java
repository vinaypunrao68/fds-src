package com.formationds.nativeapi;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class BucketStatsContent {
    private String bucketName;
    private int priority;
    private double performance;
    private double sla;
    private double limit;

    public BucketStatsContent(String bucketName, int priority, double performance, double sla, double limit) {
        this.bucketName = bucketName;
        this.priority = priority;
        this.performance = performance;
        this.sla = sla;
        this.limit = limit;
    }

    public String getBucketName() {
        return bucketName;
    }

    public int getPriority() {
        return priority;
    }

    public double getPerformance() {
        return performance;
    }

    public double getSla() {
        return sla;
    }

    public double getLimit() {
        return limit;
    }

    @Override
    public String toString() {
        return "BucketStatsContent{" +
                "bucketName='" + bucketName + '\'' +
                ", priority=" + priority +
                ", performance=" + performance +
                ", sla=" + sla +
                ", limit=" + limit +
                '}';
    }
}
