package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.Configuration;
import org.junit.Test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class AuthenticatorTest {
    @Test
    public void testLogin() throws Exception {
        Configuration configuration = new Configuration(new String[0]);

        JaasAuthenticator authenticator = new JaasAuthenticator();
        assertFalse(authenticator.login("foo", "bar"));
        assertTrue(authenticator.login("fds", "fds"));
    }
}
