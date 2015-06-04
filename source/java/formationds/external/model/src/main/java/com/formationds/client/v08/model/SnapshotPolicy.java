/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;

import java.time.Duration;

public class SnapshotPolicy extends AbstractResource<Long> {


    public enum SnapshotPolicyType { USER, SYSTEM_TIMELINE }

    private SnapshotPolicyType type;
    private Duration       retentionTime;
    private RecurrenceRule recurrenceRule;

    SnapshotPolicy() {}

    public SnapshotPolicy( SnapshotPolicyType type,
                           RecurrenceRule recurrenceRule,
                           Duration retentionTime ) {
        this( null, null, type, recurrenceRule, retentionTime );
    }

    public SnapshotPolicy( String policyName,
                           SnapshotPolicyType type,
                           RecurrenceRule recurrenceRule,
                           Duration retentionTime ) {
        this( null, policyName, type, recurrenceRule, retentionTime );
    }

    public SnapshotPolicy( Long id,
                           String policyName,
                           SnapshotPolicyType type,
                           RecurrenceRule recurrenceRule,
                           Duration retentionTime ) {
        super(id, policyName);
        this.type = type;
        this.retentionTime = retentionTime;
        this.recurrenceRule = recurrenceRule;
    }

    public SnapshotPolicyType getType() { return type; }
    public void setType(SnapshotPolicyType t) { this.type = t; }

    public Duration getRetentionTime() {
        return retentionTime;
    }

    public SnapshotPolicy withRetentionTime( Duration retentionTime ) {
        this.retentionTime = retentionTime;
        return this;
    }

    public RecurrenceRule getRecurrenceRule() {
        return recurrenceRule;
    }

    public SnapshotPolicy withRecurrenceRule( RecurrenceRule recurrenceRule ) {
        this.recurrenceRule = recurrenceRule;
        return this;
    }
}
