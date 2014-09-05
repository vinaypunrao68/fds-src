package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.xdi.CachedConfiguration;

import java.util.function.Supplier;

public class FdsAuthorizer implements Authorizer {
    private Supplier<CachedConfiguration> configSupplier;

    public FdsAuthorizer(Supplier<CachedConfiguration> configSupplier) {
        this.configSupplier = configSupplier;
    }

    @Override
    public long tenantId(AuthenticationToken token) throws SecurityException {
        User user = userFor(token);
        return configSupplier.get().tenantId(user.getId());
    }

    @Override
    public boolean hasAccess(AuthenticationToken token, String volume) throws SecurityException {
        User user = userFor(token);
        if (user.isIsFdsAdmin()) {
            return true;
        }
        return configSupplier.get().hasAccess(user.getId(), volume);
    }

    @Override
    public User userFor(AuthenticationToken token) throws SecurityException {
        return configSupplier.get().users().stream()
                .filter(u -> u.getId() == token.getUserId())
                .filter(u -> u.getSecret().equals(token.getSecret()))
                .findFirst()
                .orElseThrow(SecurityException::new);
    }
}
