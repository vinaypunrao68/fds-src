/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.security;

import com.formationds.apis.User;

public interface Authorizer {

    /**
     * This API is two-part.  First, it determines that the user authentication token
     * is valid via #userFor(AuthenticationToken).  Then it returns the tenant id
     * the user is associated with.
     *
     * @param token
     * @return the tenant id
     * @throws SecurityException if the token is not valid.
     */
    long tenantId(AuthenticationToken token) throws SecurityException;

    /**
     *
     * @param token
     * @param volume
     * @return
     */
    boolean ownsVolume(AuthenticationToken token, String volume);

    /**
     *
     * @param token
     * @return
     * @throws SecurityException
     */
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
        if (!ownsVolume(token, volume)) {
            throw new SecurityException();
        }
        return true;
    }
}
