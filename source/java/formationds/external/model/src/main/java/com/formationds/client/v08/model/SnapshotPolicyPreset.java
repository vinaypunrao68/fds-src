/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;

import java.time.Duration;

public class SnapshotPolicyPreset extends SnapshotPolicyBase {

    public SnapshotPolicyPreset( Duration retentionTime, RecurrenceRule recurrenceRule ) {
        super(retentionTime, recurrenceRule);
    }

    public SnapshotPolicyPreset( Long id, Duration retentionTime, RecurrenceRule recurrenceRule ) {
        super(id, retentionTime, recurrenceRule);
    }
}
