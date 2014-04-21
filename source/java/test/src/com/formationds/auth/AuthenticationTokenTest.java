package com.formationds.auth;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.sun.security.auth.UserPrincipal;
import org.junit.Test;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import java.security.Principal;

import static org.junit.Assert.*;

public class AuthenticationTokenTest {

    @Test
    public void testEncryptDecrypt() {
        SecretKey secretKey = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");
        Principal principal = new UserPrincipal("admin");
        AuthenticationToken token = new AuthenticationToken(secretKey, principal);
        assertNotEquals(token.toString(), token.toString());
        String encrypted = token.toString();
        AuthenticationToken thawed = new AuthenticationToken(secretKey, encrypted);
        assertTrue(thawed.isValid());
        assertFalse(new AuthenticationToken(secretKey, "garbage").isValid());
    }
}
