package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

import static org.junit.Assert.*;

public class AuthenticationTokenTest {
    public static final SecretKey SECRET_KEY = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");

    @Test
    public void testSign() {
        String secret = "secret";
        long userId = 42;
        AuthenticationToken token = new AuthenticationToken(SECRET_KEY, userId, secret);
        String tokenSignature = token.signature();
        assertEquals("2PduLDh49ZSWbKHbAqt1tJO3HCHI+ypbJy28qdTfjNA=", tokenSignature);
        assertTrue(AuthenticationToken.isValid(SECRET_KEY, tokenSignature));
    }

    @Test(expected = SecurityException.class)
    public void testCheckSignatureFails() {
        assertFalse(AuthenticationToken.isValid(SECRET_KEY, "blah"));
        new AuthenticationToken(SECRET_KEY, "blah");
    }

    @Test
    public void testCheckSignatureSucceeds() {
        String secret = "secret";
        long userId = 42;
        String tokenSignature = new AuthenticationToken(SECRET_KEY, 42, secret).signature();
        AuthenticationToken token = new AuthenticationToken(SECRET_KEY, tokenSignature);
        assertEquals(secret, token.getSecret());
        assertEquals(userId, token.getUserId());
    }

}
