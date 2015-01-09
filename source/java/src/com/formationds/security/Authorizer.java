package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;

public interface Authorizer {
    long tenantId(AuthenticationToken token) throws SecurityException;
    boolean hasAccess(AuthenticationToken token, String volume);
    User userFor(AuthenticationToken token) throws SecurityException;

    /**
     * Check that the user identified by the token has access to the specified volume throwing
     * a SecurityException if not..
     * <p/>
     * NOTE: Domain is currently not supported
     * @param token
     * @param domain
     * @param volume
     * @return true if the user identified by the token has access to the volume
     * @throws SecurityException if the user does not have access
     */
    // TODO: domain not supported yet
    default boolean checkAccess(AuthenticationToken token, String domain, String volume) throws SecurityException {
        if (!hasAccess(token, volume)) {
            throw new SecurityException();
        }
        return true;
    }
}
