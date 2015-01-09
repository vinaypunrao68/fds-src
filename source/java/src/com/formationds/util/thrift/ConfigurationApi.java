/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.util.thrift;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.security.AuthenticationToken;
import org.apache.thrift.TException;

import java.util.Collection;
import java.util.Optional;

/**
 * additional configuration read operatins.
 */
// TODO: may want to move this out of thrift utils and abstract from the thrift ConfigurationService
public interface ConfigurationApi extends ConfigurationService.Iface {

    long createSnapshotPolicy(String name,
                              String recurrence,
                              long retention,
                              long timelineTime) throws TException;

    Collection<User> listUsers();

    Optional<Tenant> tenantFor(long userId);

    Long tenantId(long userId) throws SecurityException;

    boolean hasAccess(long userId, String volumeName);

    User userFor(AuthenticationToken token) throws SecurityException;

    User getUser(long userId);

    User getUser(String login);
}
