package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.svc.types.FDSP_Uuid;

import java.io.*;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;

public class Dlt {
    private long version;
    private int numBitsForToken;
    private int depth;
    private int numTokens;
    private FDSP_Uuid[] uuids;
    private int[][] tokenLocations;

    public Dlt(long version, int numBitsForToken, int depth, int numTokens, FDSP_Uuid[] uuids) {
        this.version = version;
        this.numBitsForToken = numBitsForToken;
        this.depth = depth;
        this.numTokens = numTokens;
        this.uuids = uuids;
        tokenLocations = new int[numTokens][depth];
    }

    public Dlt(byte[] frame) {
        try {
            DataInputStream protocol = new DataInputStream(new ByteArrayInputStream(frame));
            this.version = protocol.readLong();
            this.numBitsForToken = protocol.readInt();
            this.depth = protocol.readInt();
            this.numTokens = protocol.readInt();
            int uuidCount = protocol.readInt();
            this.uuids = new FDSP_Uuid[uuidCount];
            for (int i = 0; i < uuidCount; i++) {
                uuids[i] = new FDSP_Uuid(protocol.readLong());
            }

            tokenLocations = new int[numTokens][depth];
            boolean useByte = uuidCount <= 256;

            for (int i = 0; i < numTokens; i++) {
                for (int j = 0; j < depth; j++) {
                    tokenLocations[i][j] = (int) (useByte ? protocol.readByte() : protocol.readUnsignedShort());
                }
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public int getNumBitsForToken() {
        return numBitsForToken;
    }

    public int getDepth() {
        return depth;
    }

    public long getVersion() {
        return version;
    }

    public int getNumTokens() {
        return numTokens;
    }

    public Collection<FDSP_Uuid> getTokenPlacement(int tokenId) {
         return Arrays.stream(tokenLocations[tokenId])
                .mapToObj(i -> uuids[i])
                .collect(Collectors.toList());
    }

    public byte[] serialize() throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream daos = new DataOutputStream(baos);
        daos.writeLong(version);
        daos.writeInt(numBitsForToken);
        daos.writeInt(depth);
        daos.writeInt(numTokens);
        daos.writeInt(uuids.length);

        for (FDSP_Uuid uuid : uuids) {
            daos.writeLong(uuid.getUuid());
        }

        boolean useByte = uuids.length <= 256;
        for (int i = 0; i < numTokens; i++) {
            for (int j = 0; j < depth; j++) {
                if (useByte) {
                    daos.writeByte(tokenLocations[i][j]);
                } else {
                    daos.writeShort(tokenLocations[i][j]);
                }
            }
        }

        daos.flush();
        return baos.toByteArray();
    }

    public void placeToken(int tokenId, int position, int uuidOffset) {
        tokenLocations[tokenId][position] = uuidOffset;
    }
}

