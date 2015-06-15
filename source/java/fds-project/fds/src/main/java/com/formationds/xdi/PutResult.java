package com.formationds.xdi;

public class PutResult {
    public byte[] digest;

    public PutResult(byte[] md) {
        digest = md;
    }
}
