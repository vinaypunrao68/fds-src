package com.formationds.security;

import org.uncommons.maths.random.XORShiftRNG;

import java.util.UUID;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class FastUUID {
    // The default UUID.randomUUID() is secure and slow
    private final static XORShiftRNG RNG = new XORShiftRNG();

    public static UUID randomUUID() {
        long low = RNG.nextLong();
        long high = RNG.nextLong();
        return new UUID(low, high);
    }
}
