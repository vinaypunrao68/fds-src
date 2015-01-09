package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.CachedConfiguration;

import java.util.function.Supplier;

public class FdsAuthorizer implements Authorizer {
    private ConfigurationApi config;

    public FdsAuthorizer(ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public long tenantId(AuthenticationToken token) throws SecurityException {
        User user = userFor(token);
        if (user.isIsFdsAdmin()) {
            return 0;
        }
        return config.tenantId(user.getId());
    }

    @Override
    public boolean hasAccess(AuthenticationToken token, String volume) throws SecurityException {
        User user = userFor(token);
        if (user.isIsFdsAdmin()) {
            return true;
        }
        return config.hasAccess(user.getId(), volume);
    }

    @Override
    public User userFor(AuthenticationToken token) throws SecurityException {
        return config.userFor(token);
    }
}
