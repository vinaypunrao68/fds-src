package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;

public interface Authorizer {
    long tenantId(AuthenticationToken token) throws SecurityException;
    boolean hasAccess(AuthenticationToken token, String volume);
    User userFor(AuthenticationToken token) throws SecurityException;
}
