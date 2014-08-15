package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.joda.time.Duration;

import java.util.concurrent.ConcurrentSkipListSet;
import java.util.stream.Stream;

public class VolumeStatistics {
    private final ConcurrentSkipListSet<VolumeDatapoint> window;
    private final Duration duration;

    public VolumeStatistics(Duration duration) {
        this.duration = duration;
        window = new ConcurrentSkipListSet<>();
    }

    public int size() {
        return window.size();
    }

    public void put(VolumeDatapoint volumeDatapoint) {
        synchronized (window) {
            window.add(volumeDatapoint);
            int flushed = 0;
            do {
                flushed = flushOnce();
            } while (flushed != 0);
        }
    }

    public Stream<VolumeDatapoint> stream() {
        return window.stream();
    }

    private int flushOnce() {
        if (window.size() > 1) {
            VolumeDatapoint youngest = window.first();
            VolumeDatapoint oldest = window.last();

            long maxAge = youngest.getDateTime().getMillis() - oldest.getDateTime().getMillis();
            if (maxAge > duration.getMillis()) {
                window.remove(oldest);
                return 1;
            }
        }
        return 0;
    }
}