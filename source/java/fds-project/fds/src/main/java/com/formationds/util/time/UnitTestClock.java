package com.formationds.util.time;

import com.formationds.util.executor.ProcessExecutorSource;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public class UnitTestClock implements Clock {
    private long epochMs;
    private Set<DelayListEntry> delayList;

    public UnitTestClock(long initialMillis) {
        epochMs = initialMillis;
        delayList = new HashSet<>();

    }

    @Override
    public long currentTimeMillis() {
        return epochMs;
    }

    @Override
    public long nanoTime() {
        return epochMs * 1000 * 1000;
    }

    public void setMillisAsync(long millis) {
        synchronized (delayList) {
            epochMs = millis;
            HashSet<DelayListEntry> removeSet = new HashSet<>();
            for(DelayListEntry entry : delayList) {
                if(entry.time < epochMs) {
                    ProcessExecutorSource.getInstance().execute(() -> entry.handle.complete(null));
                    removeSet.add(entry);
                }
            }
            delayList.removeAll(removeSet);
        }
    }

    public void setMillisSync(long millis) {
        HashSet<DelayListEntry> removeSet = new HashSet<>();
        synchronized (delayList) {
            epochMs = millis;
            for(DelayListEntry entry : delayList) {
                if(entry.time < epochMs) {
                    removeSet.add(entry);
                }
            }
            delayList.removeAll(removeSet);
        }

        for(DelayListEntry entry : removeSet)
            entry.handle.complete(null);
    }

    @Override
    public CompletableFuture<Void> delay(long time, TimeUnit timeUnits) {
        if(time == 0)
            return CompletableFuture.completedFuture(null);

        CompletableFuture<Void> result = new CompletableFuture<>();
        synchronized (delayList) {
            long endTime = epochMs + TimeUnit.MILLISECONDS.convert(time, timeUnits);
            delayList.add(new DelayListEntry(endTime, result));
        }

        return result;
    }

    private class DelayListEntry {
        public long time;
        public CompletableFuture<Void> handle;

        public DelayListEntry(long time, CompletableFuture<Void> handle) {
            this.time = time;
            this.handle = handle;
        }
    }
}
