package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.joda.time.DateTime;
import org.joda.time.Duration;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class TransientState {
    private DateTime lastAccessed;
    private Optional<String> searchExpression;
    private Optional<String[]> urls;
    private Map<Throttle, Integer> throttleValues;

    public TransientState(Duration timeToLive) {
        lastAccessed = DateTime.now();
        searchExpression = Optional.empty();
        urls = Optional.empty();
        throttleValues = new HashMap<>();

        ScheduledExecutorService scavenger = Executors.newSingleThreadScheduledExecutor();
        scavenger.scheduleAtFixedRate(() -> {
            if (new Duration(lastAccessed, DateTime.now()).isLongerThan(timeToLive)) {
                searchExpression = Optional.empty();
                urls = Optional.empty();
            }
        }, 0, 5, TimeUnit.SECONDS);
        scavenger.shutdown();
    }

    public void setSearchExpression(String searchExpression) {
        lastAccessed = DateTime.now();
        this.searchExpression = Optional.of(searchExpression);
        this.urls = Optional.of(new String[]{
                "http://i.imgur.com/PMieu.jpg",
                "http://data2.whicdn.com/images/23788217/large.gif",
                "http://img.pandawhale.com/57140-FUCK-CATS-meme--dog-imgur-germ-lKyR.jpeg",
                "http://i.imgur.com/9aKhPSm.jpg",
                "http://rack.0.mshcdn.com/media/ZgkyMDE0LzAxLzAyLzMxL0NhdGJhdGhpbWd1LjY3YjczLmpwZwpwCXRodW1iCTEyMDB4NjI3IwplCWpwZw/dfe26140/ee4/Cat-bath-imgur.jpg"
        });
    }

    public Optional<String> getCurrentSearchExpression() {
        lastAccessed = DateTime.now();
        return searchExpression;
    }

    public Optional<String[]> getUrls() {
        return urls;
    }

    public int getThrottleValue(Throttle throttle) {
        return throttleValues.compute(throttle, (k, v) -> v == null ? 0 : v);
    }

    public void setThrottle(Throttle throttle, int value) {
        throttleValues.compute(throttle, (k, v) -> value);
    }
}
