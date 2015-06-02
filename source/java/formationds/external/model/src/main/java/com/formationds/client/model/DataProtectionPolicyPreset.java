/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.time.Duration;
import java.util.List;

public class DataProtectionPolicyPreset extends DataProtectionPolicyBase {
    public DataProtectionPolicyPreset( Long id, Duration commitLogRetention,
                                       List<SnapshotPolicy> snapshotPolicies ) {
        super( id, commitLogRetention, snapshotPolicies );
    }
}
