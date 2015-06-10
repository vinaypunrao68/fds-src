/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 *
 */
abstract class DataProtectionPolicyBase implements Cloneable {

    private Duration             commitLogRetention;
    private List<SnapshotPolicy> snapshotPolicies = new ArrayList<>();

    protected DataProtectionPolicyBase() {}

    /**
     * @param commitLogRetention the commit log retention period
     * @param snapshotPolicies the snapshot policies
     */
    protected DataProtectionPolicyBase( Duration commitLogRetention,
                                     List<SnapshotPolicy> snapshotPolicies ) {
        this.commitLogRetention = commitLogRetention;
        this.snapshotPolicies = new ArrayList<>( snapshotPolicies );
    }

    /**
     *
     * @return the commit log retention period
     */
    public Duration getCommitLogRetention() { return commitLogRetention; }

    /**
     *
     * @return the list of snapshot policies
     */
    public List<SnapshotPolicy> getSnapshotPolicies() { return snapshotPolicies; }

    /**
     *
     * @param commitLogRetention the commit log retention period
     */
    public void setCommitLogRetention( Duration commitLogRetention ) {
        this.commitLogRetention = commitLogRetention;
    }

    /**
     *
     * @param snapshotPolicies
     */
    public void setSnapshotPolicies( List<SnapshotPolicy> snapshotPolicies ) {
        this.snapshotPolicies = new ArrayList<>( snapshotPolicies );
    }

    public DataProtectionPolicyBase addSnapshotPolicies( SnapshotPolicy snapshotPolicy ) {
        this.snapshotPolicies.add( snapshotPolicy );
        return this;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof DataProtectionPolicyBase) ) { return false; }
        final DataProtectionPolicyBase that = (DataProtectionPolicyBase) o;
        return Objects.equals( commitLogRetention, that.commitLogRetention ) &&
               Objects.equals( snapshotPolicies, that.snapshotPolicies );
    }

    @Override
    public int hashCode() {
        return Objects.hash( commitLogRetention, snapshotPolicies );
    }
}
