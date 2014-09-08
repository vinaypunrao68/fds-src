package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Hex;

import java.security.SecureRandom;

public class GenerateAesKey {
    public static void main(String[] args) {
        SecureRandom random = new SecureRandom();
        byte[] bytes = new byte[16];
        random.nextBytes(bytes);
        System.out.println(Hex.encodeHex(bytes));
    }
}
