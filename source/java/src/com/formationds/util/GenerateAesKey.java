package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Base64;

import java.security.SecureRandom;

public class GenerateAesKey {
    public static void main(String[] args) {
        SecureRandom random = new SecureRandom();
        byte[] bytes = new byte[16];
        random.nextBytes(bytes);
        System.out.println(Base64.encodeBase64String(bytes));
    }
}
