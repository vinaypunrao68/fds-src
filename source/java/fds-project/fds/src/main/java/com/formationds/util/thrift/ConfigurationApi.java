/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.util.thrift;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.SnapshotPolicy;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import org.apache.thrift.TException;

import java.util.List;
import java.util.Optional;

/**
 * additional possibly-cached configuration read operations.
 *
 * All apis here may throw an IllegalStateException wrapping a TException on failure.
 */
// TODO: may want to move this out of thrift utils and abstract from the thrift
// ConfigurationService
public interface ConfigurationApi extends ConfigurationService.Iface {

    /**
     * Create a snapshot policy.
     * <p/>
     * This is a factory method that creates the SnapshotPolicy object then calls
     * {@link #createSnapshotPolicy(SnapshotPolicy)}
     *
     * @param name
     * @param recurrence
     * @param retention
     * @param timelineTime
     *
     * @return
     *
     * @throws TException
     */
    // TODO: not clear what the return value represents.  See createSnapshotPolicy(SnapshotPolicy)
    long createSnapshotPolicy(String name,
                              String recurrence,
                              long retention,
                              long timelineTime) throws TException;

    /**
     *
     * @param userId
     * @return the tenant that the specified user belongs to
     */
    Optional<Tenant> tenantFor(long userId);

    /**
     *
     * @param userId
     *
     * @return the tenant id that the user belongs to or null if it does not exist.
     */
    Long tenantId(long userId);

    User getUser(long userId);

    User getUser(String login);

    List<FDSP_Node_Info_Type> listLocalDomainServices( String domainName )
        throws TException;
}
