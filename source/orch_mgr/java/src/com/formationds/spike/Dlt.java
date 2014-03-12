package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.joda.time.DateTime;

public class Dlt {
    private long version;
    private DateTime timestamp;
    private int numBitsForToken;
    private int numTokens;
    private int depth;

    public Dlt(long version, int numBitsForToken, int depth, int numTokens) {
        this.version = version;
        this.numBitsForToken = numBitsForToken;
        this.depth = depth;
        this.numTokens = numTokens;
    }

    public long getVersion() {
        return version;
    }

    public int getNumBitsForToken() {
        return numBitsForToken;
    }

    public int getDepth() {
        return depth;
    }

    public int getNumTokens() {
        return numTokens;
    }
}

