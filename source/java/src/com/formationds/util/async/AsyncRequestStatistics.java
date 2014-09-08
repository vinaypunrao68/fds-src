package com.formationds.util.async;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.math.BigInteger;
import java.text.DecimalFormat;
import java.util.*;
import java.util.concurrent.CompletableFuture;

public class AsyncRequestStatistics {
    private boolean enabled;
    private final LinkedList<LatencyDataPoint> data;
    private List<String> notes;

    public class LatencyDataPoint {
        public String bucket;
        public long startTimeNs;
        public long endTimeNs;
    }

    public static class Aggregate {
        public void output(StringBuilder sb) {
            synchronized (statBucketMap) {
                for (Map.Entry<String, StatBucket> bucketEntry : statBucketMap.entrySet()) {
                    sb.append(bucketEntry.getKey());
                    sb.append(":\n");

                    StatBucket bucket = bucketEntry.getValue();
                    long avg = bucket.totalNs.divide(BigInteger.valueOf(bucket.executionCount)).longValue();

                    outputNsValue(sb, "max", Long.toString(bucket.maxNs), "ns");
                    outputNsValue(sb, "min", Long.toString(bucket.minNs), "ns");
                    outputNsValue(sb, "count", Long.toString(bucket.executionCount), "");
                    outputNsValue(sb, "avg", Long.toString(avg), "ns");
                    outputNsValue(sb, "total", bucket.totalNs.toString(), "ns");
                    sb.append("\n");
                }
            }
        }

        private void outputNsValue(StringBuilder sb, String prompt, String value, String unit) {
            sb.append(prompt);
            sb.append(": ");
            sb.append(value);
            sb.append(" ");
            sb.append(unit);
            sb.append("\n");
        }

        public class StatBucket {
            public long maxNs;
            public long minNs;
            public long executionCount;
            public BigInteger totalNs;

            public StatBucket() {
                totalNs = BigInteger.ZERO;
            }
        }

        private final Map<String, StatBucket> statBucketMap;

        public Aggregate() {
            statBucketMap = new HashMap<>();
        }

        public void add(AsyncRequestStatistics statistics) {
            if(!statistics.enabled)
                return;

            synchronized (statBucketMap) {
                for(LatencyDataPoint dp : statistics.data) {
                    StatBucket bucket = statBucketMap.computeIfAbsent(dp.bucket, (key) -> new StatBucket());
                    bucket.executionCount++;
                    long duration = dp.endTimeNs - dp.startTimeNs;
                    bucket.maxNs = Math.max(duration, bucket.maxNs);
                    bucket.minNs = Math.min(duration, bucket.maxNs);
                    bucket.totalNs = bucket.totalNs.add(BigInteger.valueOf(duration));
                }
            }
        }
    }

    public AsyncRequestStatistics(boolean enabled) {
        this.enabled = enabled;
        this.data = new LinkedList<>();
        this.notes = new LinkedList<>();
    }

    public void outputRequestLifecycle(StringBuilder sb) {
        if(enabled) {
            LinkedList<LatencyDataPoint> startPoints = new LinkedList<>();
            LinkedList<LatencyDataPoint> endPoints = new LinkedList<>();
            data.stream().sorted((x, y) -> Long.compare(x.startTimeNs, y.startTimeNs)).forEach(startPoints::addLast);
            data.stream().sorted((x, y) -> Long.compare(x.endTimeNs, y.endTimeNs)).forEach(endPoints::addLast);
            long zeroPoint = startPoints.getFirst().startTimeNs;
            while (!startPoints.isEmpty() || !endPoints.isEmpty()) {
                LatencyDataPoint dataPoint;
                if (startPoints.isEmpty() || endPoints.getFirst().endTimeNs < startPoints.getFirst().startTimeNs) {
                    LatencyDataPoint dp = endPoints.remove();
                    sb.append("[");
                    sb.append(dp.endTimeNs - zeroPoint);
                    sb.append("] ");
                    sb.append("END ");
                    sb.append(dp.bucket);
                    sb.append("\n");
                } else {
                    LatencyDataPoint dp = startPoints.remove();
                    sb.append("[");
                    sb.append(dp.startTimeNs - zeroPoint);
                    sb.append("] ");
                    sb.append("START ");
                    sb.append(dp.bucket);
                    sb.append("\n");
                }
            }
            if (notes.size() > 0) {
                sb.append("Notes:\n");
                notes.stream().forEach(n -> sb.append(n + "\n"));
            }
        }
    }

    public <T> CompletableFuture<T> time(String label, CompletableFuture<T> future) {
        if(enabled) {
            LatencyDataPoint point = new LatencyDataPoint();
            point.bucket = label;
            point.startTimeNs = System.nanoTime();
            future.whenComplete((_result, _ex) -> {
                point.endTimeNs = System.nanoTime();
                synchronized (data) {
                    data.add(point);
                }
            });
        }
        return future;
    }

    public void note(String note) {
        notes.add(note);
    }
}
