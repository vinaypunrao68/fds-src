package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Base64;

public class AuthenticationKey {
    private byte[] value;

    public AuthenticationKey(byte[] value) {
        this.value = value;
    }

    public byte[] getValue() {
        return value;
    }

    public String toBase64() {
        return Base64.encodeBase64String(value);
    }
}
