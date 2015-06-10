/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * The data protection policy for commit log retention and snapshot policies.
 */
public class DataProtectionPolicy extends DataProtectionPolicyBase {

    /**
     * Create a DataProtectionPolicy based on the specified preset.
     * @param p the preset
     * @return the new DataProtectionPolicy based on the preset
     */
    public static DataProtectionPolicy fromPreset(DataProtectionPolicyPreset p) {
        return new DataProtectionPolicy( p.getId(), p.getCommitLogRetention(), p.getSnapshotPolicies() );
    }

    private Long presetId = null;

    /**
     *
     */
    protected DataProtectionPolicy() {}

    /**
     *
     * @param commitLogRetention the commit log retention period
     * @param snapshotPolicies the list of snapshot policies for the volume protection
     */
    public DataProtectionPolicy( Duration commitLogRetention,
                                 List<SnapshotPolicy> snapshotPolicies ) {
        super(commitLogRetention, snapshotPolicies);
    }

    /**
     *
     * @param presetId the preset this policy is based on
     * @param commitLogRetention the commit log retention period
     * @param snapshotPolicies the list of snapshot policies for the volume protection
     */
    public DataProtectionPolicy( Long presetId, Duration commitLogRetention,
                                 List<SnapshotPolicy> snapshotPolicies ) {
        super( commitLogRetention, snapshotPolicies );
        this.presetId = presetId;
    }

    /**
     *
     * @return a copy of this policy and it's snapshot policies.  The snapshot policy names and ids will be null as
     * they are generated when saved.
     */
    public DataProtectionPolicy newPolicyFrom() {
        List<SnapshotPolicy> spcp = new ArrayList<>();
        getSnapshotPolicies().forEach( (sp -> { spcp.add( sp.newPolicyFrom() ); }) );
        return new DataProtectionPolicy( presetId, getCommitLogRetention(), spcp );
    }

    /**
     * @return the id of the preset this policy is based on
     */
    public Long getPresetId() {
        return presetId;
    }

    /**
     *
     * @param presetId the id of the preset this policy is based on
     */
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
