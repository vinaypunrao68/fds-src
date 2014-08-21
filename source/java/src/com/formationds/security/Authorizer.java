package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;

public interface Authorizer {
    boolean hasAccess(User user, String volumeName);
}
