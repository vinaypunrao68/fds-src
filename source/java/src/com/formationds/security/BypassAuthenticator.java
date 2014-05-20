package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.security.auth.login.LoginException;

public class BypassAuthenticator implements Authenticator {
    @Override
    public void login(String login, String password) throws LoginException {

    }
}
