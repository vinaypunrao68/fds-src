package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.MinMaxPriorityQueue;
import org.joda.time.Duration;

import java.util.Collection;
import java.util.stream.Collectors;
/**
 * @deprecated the POC functionality has been replaced.
 *
 *             will be removed in the future
 */
@Deprecated
public class VolumeStatistics {
    private final MinMaxPriorityQueue<VolumeDatapoint> window;
    private final Duration duration;

    public VolumeStatistics(Duration duration) {
        this.duration = duration;
        window = MinMaxPriorityQueue.create();
    }

    public int size() {
        synchronized (window) {
            return window.size();
        }
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

    public VolumeDatapoint[] datapoints() {
        synchronized (window) {
            return window.toArray(new VolumeDatapoint[0]);
        }
    }

    public VolumeDatapoint[] datapoints(String volumeName) {
        synchronized (window) {
            return window.stream()
                    .filter(vdp -> volumeName.equals(vdp.getVolumeName()))
                    .toArray(i -> new VolumeDatapoint[i]);
        }
    }
    public Collection<String> volumes() {
        synchronized (window) {
            return window.stream()
                    .map(v -> v.getVolumeName())
                    .distinct()
                    .sorted((l, r) -> l.compareTo(r))
                    .collect(Collectors.toList());
        }
    }

    private int flushOnce() {
        if (window.size() > 1) {
            VolumeDatapoint youngest = window.peekFirst();
            VolumeDatapoint oldest = window.peekLast();

            long maxAge = youngest.getDateTime().getMillis() - oldest.getDateTime().getMillis();
            if (maxAge > duration.getMillis()) {
                window.remove(oldest);
                return 1;
            }
        }
        return 0;
    }

}