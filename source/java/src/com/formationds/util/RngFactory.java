package com.formationds.util;

import com.formationds.smoketest.S3SmokeTest;

import java.security.SecureRandom;
import java.util.Random;

public class RngFactory {
    public static final Random loadRNG() {
        final String rngClassName = System.getProperty(S3SmokeTest.RNG_CLASS);
        if (rngClassName == null) {
            return new Random();
        } else {
            switch (rngClassName) {
                case "java.util.Random":
                    return new Random();
                case "java.security.SecureRandom":
                    return new SecureRandom();
                case "java.util.concurrent.ThreadLocalRandom":
                    throw new IllegalArgumentException(
                            "ThreadLocalRandom is not supported - can't instantiate (must use ThreadLocalRandom.current())");
                default:
                    try {
                        Class<?> rngClass = Class.forName(rngClassName);
                        return (Random) rngClass.newInstance();
                    } catch (ClassNotFoundException | InstantiationException
                            | IllegalAccessException cnfe) {
                        throw new IllegalStateException(
                                "Failed to instantiate Random implementation specified by \""
                                        + S3SmokeTest.RNG_CLASS + "\"system property: "
                                        + rngClassName, cnfe);
                    }
            }
        }
    }
}
