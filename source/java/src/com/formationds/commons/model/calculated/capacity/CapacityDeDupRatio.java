/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.calculated.capacity;

import com.formationds.commons.model.abs.Calculated;
import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */

public class CapacityDeDupRatio
  extends Calculated {
  private static final long serialVersionUID = 455524997304050359L;

  @SerializedName("dedup_ratio")
  private Double ratio;

  /**
   * @param ratio the {@link Double} representing the de-duplication ratio
   */
  public CapacityDeDupRatio( final Double ratio ) {
    this.ratio = ratio;
  }

  /**
   * @return Returns {@link Double} representing the de-duplication ratio
   */
  public Double getRatio() {
    return ratio;
  }
}
