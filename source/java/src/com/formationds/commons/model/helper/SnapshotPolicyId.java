/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class SnapshotPolicyId
  extends ModelBase {
  private long snapshotPolicyId;

  /**
   * default constructor
   */
  public SnapshotPolicyId() {
  }

  /**
   * @return Returns {@code long} representing the snapshot policy unique identifier
   */
  public long getSnapshotPolicyId() {
    return snapshotPolicyId;
  }

  /**
   * @param snapshotPolicyId the {@code long} representing the snapshot policy unique identifier
   */
  public void setSnapshotPolicyId( final long snapshotPolicyId ) {
    this.snapshotPolicyId = snapshotPolicyId;
  }
}
