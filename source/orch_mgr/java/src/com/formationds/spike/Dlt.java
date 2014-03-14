package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_Uuid;
import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Multimap;
import org.joda.time.DateTime;

import java.util.Collection;
import java.util.List;

public class Dlt {
    private long version;
    private DateTime timestamp;
    private int numBitsForToken;
    private int numTokens;
    private int depth;
    private List<FDSP_Uuid> uuids;
    private Multimap<Integer, FDSP_Uuid> placement;

    public Dlt(long version, int numBitsForToken, int depth, int numTokens, List<FDSP_Uuid> uuids) {
        this.version = version;
        this.numBitsForToken = numBitsForToken;
        this.depth = depth;
        this.numTokens = numTokens;
        this.uuids = uuids;
        placement = ArrayListMultimap.create();
    }

    public void addPlacement(int token, int uuidOffset) {
        FDSP_Uuid uuid = uuids.get(uuidOffset);
        placement.put(token, uuid);
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

    public List<FDSP_Uuid> getUuids() {
        return uuids;
    }

    public Collection<FDSP_Uuid> getTokenPlacement(int tokenId) {
        return placement.get(tokenId);
    }
}

