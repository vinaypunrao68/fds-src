package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;

import java.util.Collection;

public class CachedConfiguration {
    public Collection<User> users() {
        return null;
    }

    public Collection<User> usersForTenant(long tenantId) {
        return null;
    }

    public long tenantId(long userId) throws SecurityException {
        return 0;
    }

    public boolean hasAccess(long userId, String volumeName) {
        return false;
    }
}
