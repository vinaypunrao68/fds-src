package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;

public class DumbAuthorizer implements Authorizer {
    @Override
    public long tenantId(AuthenticationToken token) throws SecurityException {
        return 0;
    }

    @Override
    public boolean ownsVolume(AuthenticationToken token, String volume) {
        return true;
    }

    @Override
    public User userFor(AuthenticationToken token) throws SecurityException {
        return new User(0, "admin", "", "", true);
    }
}
