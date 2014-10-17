/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.capacity;

import com.formationds.commons.model.abs.Calculated;

/**
 * @author ptinius
 */
public class CapacityDeDupRatio
  extends Calculated {
  private Double ratio;

  /**
   * default constructor
   */
  public CapacityDeDupRatio() {
  }

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

  /**
   * @param ratio the {@link Double} representing the de-duplication ratio;
   */
  public void setRatio( final Double ratio ) {
    this.ratio = ratio;
  }
}
