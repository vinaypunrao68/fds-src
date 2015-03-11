package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.security.auth.login.LoginException;

public interface Authenticator {
    public boolean allowAll();

    AuthenticationToken authenticate(String login, String password) throws LoginException;

    AuthenticationToken currentToken(String login) throws LoginException;

    AuthenticationToken reissueToken(long userId) throws LoginException;

    AuthenticationToken parseToken(String signature) throws LoginException;
}
