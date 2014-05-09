package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.sun.security.auth.UserPrincipal;
import org.junit.Test;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import java.security.Principal;

import static org.junit.Assert.*;

public class AuthorizationTokenTest {

    @Test
    public void testEncryptDecrypt() {
        SecretKey secretKey = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");
        Principal principal = new UserPrincipal("admin");
        AuthorizationToken token = new AuthorizationToken(secretKey, principal);
        assertNotEquals(token.getKey().toBase64(), token.getKey().toBase64());
        String encrypted = token.getKey().toBase64();
        AuthorizationToken thawed = new AuthorizationToken(secretKey, encrypted);
        assertTrue(thawed.isValid());
        assertEquals("admin", thawed.getName());
        assertFalse(new AuthorizationToken(secretKey, "garbage").isValid());
    }
}
