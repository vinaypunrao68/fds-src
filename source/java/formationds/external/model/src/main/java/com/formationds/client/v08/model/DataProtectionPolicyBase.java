/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

abstract class DataProtectionPolicyBase {

    private Duration             commitLogRetention;
    private List<SnapshotPolicy> snapshotPolicies;

    public DataProtectionPolicyBase( Duration commitLogRetention,
                                     List<SnapshotPolicy> snapshotPolicies ) {
        this.commitLogRetention = commitLogRetention;
        this.snapshotPolicies = new ArrayList<>( snapshotPolicies );
    }

    public Duration getCommitLogRetention() { return commitLogRetention; }
    public List<SnapshotPolicy> getSnapshotPolicies() { return snapshotPolicies; }

    public void setCommitLogRetention( Duration commitLogRetention ) {
        this.commitLogRetention = commitLogRetention;
    }

    public void setSnapshotPolicies( List<SnapshotPolicy> snapshotPolicies ) {
        this.snapshotPolicies = new ArrayList<>( snapshotPolicies );
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
