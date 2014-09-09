package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

import static org.junit.Assert.assertEquals;

public class AuthenticationTokenTest {
    public static final SecretKey SECRET_KEY = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");

    @Test
    public void testSign() {
        String secret = "secret";
        long userId = 42;
        AuthenticationToken token = new AuthenticationToken(userId, secret);
        String tokenSignature = token.signature(SECRET_KEY);
        assertEquals("202c06c0f6d3b8c6165fb3bda72abe1f9dfe4b52342f688623ae826b83dfd779", tokenSignature);
        new TokenEncrypter().tryParse(SECRET_KEY, tokenSignature);
    }

    @Test(expected = SecurityException.class)
    public void testCheckSignatureFails() {
        new TokenEncrypter().tryParse(SECRET_KEY, "blah");
    }

    @Test
    public void testCheckSignatureSucceeds() {
        String secret = "secret";
        long userId = 42;
        String tokenSignature = new AuthenticationToken(42, secret).signature(SECRET_KEY);
        AuthenticationToken result = new TokenEncrypter().tryParse(SECRET_KEY, tokenSignature);
        assertEquals(secret, result.getSecret());
        assertEquals(userId, result.getUserId());
    }

}
