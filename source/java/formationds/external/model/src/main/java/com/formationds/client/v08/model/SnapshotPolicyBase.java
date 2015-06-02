/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;

import java.time.Duration;

abstract class SnapshotPolicyBase {
    private Long             id;
    private Duration       retentionTime;
    private RecurrenceRule recurrenceRule;

    public SnapshotPolicyBase( Duration retentionTime, RecurrenceRule recurrenceRule ) {
        this.retentionTime = retentionTime;
        this.recurrenceRule = recurrenceRule;
    }

    public SnapshotPolicyBase( Long id, Duration retentionTime, RecurrenceRule recurrenceRule ) {
        this.id = id;
        this.retentionTime = retentionTime;
        this.recurrenceRule = recurrenceRule;
    }

    public Long getId() {
        return id;
    }

    public SnapshotPolicyBase withId( Long id ) {
        this.id = id;
        return this;
    }

    public Duration getRetentionTime() {
        return retentionTime;
    }

    public SnapshotPolicyBase withRetentionTime( Duration retentionTime ) {
        this.retentionTime = retentionTime;
        return this;
    }

    public RecurrenceRule getRecurrenceRule() {
        return recurrenceRule;
    }

    public SnapshotPolicyBase withRecurrenceRule( RecurrenceRule recurrenceRule ) {
        this.recurrenceRule = recurrenceRule;
        return this;
    }
}
