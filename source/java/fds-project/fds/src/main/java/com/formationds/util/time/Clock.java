package com.formationds.util.time;

import org.joda.time.DateTime;

public interface Clock {
    long currentTimeMillis();
    long nanoTime();
    DateTime now();
}
