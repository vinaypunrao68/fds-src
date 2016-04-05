package com.formationds.util;

import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;
import org.joda.time.Duration;

import java.util.concurrent.ExecutionException;

public class Retry<In, Out> implements FunctionWithExceptions<In, Out> {
    private static final Logger LOG = LogManager.getLogger(Retry.class);

    private BiFunctionWithExceptions<In, Integer, Out> function;
    private int maxTries;
    private Duration delay;
    private String actionName;

    public Retry(BiFunctionWithExceptions<In, Integer, Out> function, int maxTries, Duration delayBetweenAttempts, String actionName) {
        this.function = function;
        this.maxTries = maxTries;
        delay = delayBetweenAttempts;
        this.actionName = actionName;
    }

    @Override
    public Out apply(In input) throws Exception {
        Exception last = null;

        for (int i = 0; i < maxTries; i++) {
            try {
                return function.apply(input, i);
            } catch (Exception e) {
                LOG.warn("Attempting [" + actionName + "] failed, " + (maxTries - i) + " attempts left");
                last = e;
                Thread.sleep(delay.getMillis());
            }
        }

        String message = "[" + actionName + "]: execution failed after " + maxTries + " attempts";
        LOG.error(message, last);
        throw new ExecutionException(message, last);
    }
}
