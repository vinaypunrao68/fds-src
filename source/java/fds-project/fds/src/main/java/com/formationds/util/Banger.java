package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.math3.stat.descriptive.SummaryStatistics;
import org.apache.commons.math3.stat.descriptive.SynchronizedSummaryStatistics;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;

public class Banger {
    private int bangsPerCycle;
    private int maxThreadCount;
    private int step;
    private Supplier<Runnable> supplier;

    public Banger(int bangsPerCycle, int maxThreadCount, int step, Supplier<Runnable> supplier) {
        this.bangsPerCycle = bangsPerCycle;
        this.maxThreadCount = maxThreadCount;
        this.step = step;
        this.supplier = supplier;
    }

    public Map<Integer, SummaryStatistics> bang() {
        HashMap<Integer, SummaryStatistics> result = new HashMap<>();
        for (int i = 1; i <= maxThreadCount; i += step) {
            result.put(i, bang(i));
        }
        return result;
    }

    private SummaryStatistics bang(int threadCount) {
        SummaryStatistics stats = new SynchronizedSummaryStatistics();
        ExecutorService executor = Executors.newFixedThreadPool(threadCount);
        for (int i = 0; i < threadCount; i++) {
            executor.submit((Runnable)() -> {
                for (int j = 0; j < bangsPerCycle; j++) {
                    Runnable task = supplier.get();
                    long then = System.currentTimeMillis();
                    task.run();
                    long elapsed = System.currentTimeMillis() - then;
                    stats.addValue(elapsed);
                }
            });
        }
        executor.shutdown();
        try {
            executor.awaitTermination(1, TimeUnit.DAYS);
        } catch (InterruptedException e) {
        }

        return stats;
    }

}
