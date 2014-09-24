/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class VolumeId
  extends ModelBase {
  private long volumeId;

  /**
   * default constructor
   */
  public VolumeId() {
  }

  /**
   * @return Returns {@code long} representing the volume unique identifier
   */
  public long getVolumeId() {
    return volumeId;
  }

  /**
   * @param volumeId the {@code long} representing the volume unique identifier
   */
  public void setVolumeId( long volumeId ) {
    this.volumeId = volumeId;
  }
}
