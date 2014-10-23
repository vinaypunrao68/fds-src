/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.capacity;

import com.formationds.commons.model.abs.Calculated;
import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */

public class CapacityDeDupRatio
  extends Calculated {
  private static final long serialVersionUID = 455524997304050359L;

  @SerializedName( "volumeName" )
  private String volumeName;
  @SerializedName("ratio")
  private Double ratio;

  /**
   * @param volumeName the {@link String} representing the name of the volume
   * @param ratio the {@link Double} representing the de-duplication ratio
   */
  public CapacityDeDupRatio( final String volumeName, final Double ratio ) {
    this.volumeName = volumeName;
    this.ratio = ratio;
  }

  /**
   * @return Returns {@link Double} representing the de-duplication ratio
   */
  public Double getRatio() {
    return ratio;
  }

  /**
   * @return Returns {@link String} representing the name of the volume
   */
  public String getVolumeName() {
    return volumeName;
  }
}
