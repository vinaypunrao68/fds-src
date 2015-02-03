/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.security;

import com.formationds.apis.ApiException;
import com.formationds.apis.ErrorCode;
import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.util.thrift.ConfigurationApi;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class FdsAuthorizer implements Authorizer {
    private static final Logger logger = LoggerFactory.getLogger(FdsAuthorizer.class);

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
    public boolean ownsVolume(AuthenticationToken token, String volume) throws SecurityException {
        if (AuthenticationToken.ANONYMOUS.equals(token)) {
            return false;
        }
        try {
            VolumeDescriptor v = config.statVolume("", volume);
            if (v == null) {
                if (logger.isTraceEnabled()) {
                    logger.trace("AUTHZ::HASACCESS::DENIED " + volume + " not found.");
                }
                return false;
            }

            User user = userFor(token);
            if (user.isIsFdsAdmin()) {
                if (logger.isTraceEnabled()) {
                    logger.trace("AUTHZ::HASACCESS::GRANTED " + volume + " (admin)");
                }
                return true;
            }

            long volTenantId = v.getTenantId();
            long userTenantId = tenantId(token);

            if (userTenantId == volTenantId) {
                if (logger.isTraceEnabled()) {
                    logger.trace("AUTHZ::HASACCESS::GRANTED volume={} userid={}", volume, token.getUserId());
                }

                return true;
            } else {
                if (logger.isTraceEnabled()) {
                    logger.trace("AUTHZ::HASACCESS::DENIED volume={} userid={}", volume, token.getUserId());
                }

                return false;
            }
        } catch (ApiException apie) {
            if (apie.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                if (logger.isTraceEnabled()) {
                    logger.trace("AUTHZ::HASACCESS::DENIED " + volume + " not found.");
                }
                return false;
            }
            throw new IllegalStateException("Failed to access config service.", apie);

        } catch (TException e) {
            throw new IllegalStateException("Failed to access config service.", e);
        }
    }

    @Override
    public User userFor(AuthenticationToken token) throws SecurityException {
        User user = config.getUser(token.getUserId());
        if (user == null) {
            if (logger.isTraceEnabled()) {
                logger.trace("AUTHZ::HASACCESS::DENIED user userid={} not found.", token.getUserId());
            }

            throw new SecurityException();
        } else if (!user.getSecret().equals(token.getSecret())) {
            throw new SecurityException();
        }
        return user;
    }
}
