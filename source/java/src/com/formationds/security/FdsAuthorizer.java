/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.security;

import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.commons.model.Volume;
import com.formationds.util.thrift.ConfigurationApi;
import org.apache.thrift.TException;

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
        try {
            VolumeDescriptor v = config.statVolume("", volume);
            if (v == null) {
                return false;
            }

            User user = userFor(token);
            if (user.isIsFdsAdmin()) {
                return true;
            }

            long tenantId = v.getTenantId();

            return config.tenantId(user.getId()) == tenantId;

        } catch (TException e) {
            throw new IllegalStateException("Failed to access server.", e);
        }
    }

    @Override
    public User userFor(AuthenticationToken token) throws SecurityException {
        User user = config.getUser(token.getUserId());
        if (user == null || !user.getSecret().equals(token.getSecret())) {
            throw new SecurityException();
        }
        return user;
    }
}
