/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.time.Duration;
import java.util.List;
import java.util.Objects;

public class DataProtectionPolicyPreset extends DataProtectionPolicyBase {
    private Long id;

    public DataProtectionPolicyPreset( Duration commitLogRetention,
                                       List<SnapshotPolicy> snapshotPolicies ) {
        super( commitLogRetention, snapshotPolicies );
    }
    public DataProtectionPolicyPreset( Long id, Duration commitLogRetention,
                                       List<SnapshotPolicy> snapshotPolicies ) {
        super( commitLogRetention, snapshotPolicies );
        this.id = id;
    }

    public Long getId() { return id; }
    public void setId( Long id ) {
        this.id = id;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof DataProtectionPolicyPreset) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final DataProtectionPolicyPreset that = (DataProtectionPolicyPreset) o;
        return Objects.equals( id, that.id );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), id );
    }
}
