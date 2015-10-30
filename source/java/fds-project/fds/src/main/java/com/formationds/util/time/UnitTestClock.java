package com.formationds.util.time;

public class UnitTestClock implements Clock {
    long epochMs;

    @Override
    public long currentTimeMillis() {
        return epochMs;
    }

    @Override
    public long nanoTime() {
        return epochMs * 1000 * 1000;
    }

    public void setMillis(long millis) {
        epochMs = millis;
    }
}
