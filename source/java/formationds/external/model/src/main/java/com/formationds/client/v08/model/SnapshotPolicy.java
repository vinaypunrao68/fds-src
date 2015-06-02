/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;

import java.time.Duration;

public class SnapshotPolicy {

    private Long           id;
    private Duration       retentionTime;
    private RecurrenceRule recurrenceRule;

    public SnapshotPolicy( RecurrenceRule recurrenceRule, Duration retentionTime ) {
        this.retentionTime = retentionTime;
        this.recurrenceRule = recurrenceRule;
    }

    public SnapshotPolicy( Long id, RecurrenceRule recurrenceRule, Duration retentionTime ) {
        this.id = id;
        this.retentionTime = retentionTime;
        this.recurrenceRule = recurrenceRule;
    }

    public Long getId() {
        return id;
    }

    public SnapshotPolicy withId( Long id ) {
        this.id = id;
        return this;
    }

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
