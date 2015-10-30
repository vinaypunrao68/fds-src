package com.formationds.util.time;

import com.formationds.util.Lazy;

public class SystemClock implements Clock {
    private static Lazy<SystemClock> lazy = new Lazy<>(SystemClock::new);
    public static Clock current() {
        return lazy.getInstance();
    }

    @Override
    public long currentTimeMillis() {
        return System.currentTimeMillis();
    }

    @Override
    public long nanoTime() {
        return System.nanoTime();
    }

}
