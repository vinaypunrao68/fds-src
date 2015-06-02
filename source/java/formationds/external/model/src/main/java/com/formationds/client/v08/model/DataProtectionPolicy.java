/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.time.Duration;
import java.util.List;
import java.util.Objects;

public class DataProtectionPolicy extends DataProtectionPolicyBase {
    public static DataProtectionPolicy fromPreset(DataProtectionPolicyPreset p) {
        return new DataProtectionPolicy( p.getId(), p.getCommitLogRetention(), p.getSnapshotPolicies() );
    }

    private Long presetId = null;
    private Volume volume;
    public DataProtectionPolicy( Duration commitLogRetention,
                                 List<SnapshotPolicy> snapshotPolicies ) {
        super(commitLogRetention, snapshotPolicies);
    }
    public DataProtectionPolicy( Long presetId, Duration commitLogRetention,
                                 List<SnapshotPolicy> snapshotPolicies ) {
        super( commitLogRetention, snapshotPolicies );
        this.presetId = presetId;
    }

    public Long getPresetId() {
        return presetId;
    }

    public void setPresetId( Long presetId ) {
        this.presetId = presetId;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof DataProtectionPolicy) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final DataProtectionPolicy that = (DataProtectionPolicy) o;
        return Objects.equals( presetId, that.presetId );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), presetId );
    }
}
