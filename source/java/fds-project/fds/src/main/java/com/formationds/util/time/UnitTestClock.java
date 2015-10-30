package com.formationds.util.time;

import org.joda.time.DateTime;

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

    @Override
    public DateTime now() {
        return new DateTime(epochMs);
    }

    public void setDate(DateTime current) {
        epochMs = current.getMillis();
    }

    public void setMillis(long millis) {
        epochMs = millis;
    }
}
