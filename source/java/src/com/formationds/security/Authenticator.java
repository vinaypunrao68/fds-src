package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import javax.security.auth.login.LoginException;

public interface Authenticator {
    SecretKey KEY = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");

    AuthenticationToken reissueToken(String login, String password) throws LoginException;

    AuthenticationToken resolveToken(String signature) throws LoginException;
}
