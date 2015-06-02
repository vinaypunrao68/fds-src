/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;

import java.time.Duration;

public class SnapshotPolicy extends SnapshotPolicyBase {

    public static SnapshotPolicy fromPreset(SnapshotPolicyBase p) {
        return new SnapshotPolicy( p.getId(), p.getRecurrenceRule(), p.getRetentionTime() );
    }

    private Long presetId;

    public SnapshotPolicy( RecurrenceRule recurrenceRule, Duration retentionTime ) {
        super(retentionTime, recurrenceRule);
    }

    public SnapshotPolicy( Long presetId, RecurrenceRule recurrenceRule, Duration retentionTime ) {
        super(retentionTime, recurrenceRule);
        this.presetId = presetId;
    }

    public SnapshotPolicy( Long id, Long presetId, RecurrenceRule recurrenceRule, Duration retentionTime ) {
        super(id, retentionTime, recurrenceRule );
        this.presetId = presetId;
    }

    public Long getPresetId() { return presetId; }

    public SnapshotPolicy withPresetId( Long presetId ) {
        this.presetId = presetId;
        return this;
    }
}
