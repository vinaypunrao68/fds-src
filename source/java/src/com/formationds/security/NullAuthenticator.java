package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.security.auth.login.LoginException;

public class NullAuthenticator implements Authenticator {
    @Override
    public boolean allowAll() {
        return true;
    }

    @Override
    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        return AuthenticationToken.ANONYMOUS;
    }

    @Override
    public AuthenticationToken currentToken(String login) throws LoginException {
        return AuthenticationToken.ANONYMOUS;
    }

    @Override
    public AuthenticationToken reissueToken(long userId) throws LoginException {
        throw new UnsupportedOperationException();
    }

    @Override
    public AuthenticationToken parseToken(String signature) throws LoginException {
        return AuthenticationToken.ANONYMOUS;
    }
}
