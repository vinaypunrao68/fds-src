/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.capacity.builder;

import com.formationds.commons.model.capacity.CapacityDeDupRatio;

/**
 * @author ptinius
 */
public class CapacityDeDupRatioBuilder {
  private String volumeName;
  private Double ratio;

  /**
   * default constructor
   */
  public CapacityDeDupRatioBuilder() {
  }

  /**
   * @param ratio the {@link Double} representing the de-duplication ratio
   *
   * @return Returns the {@link com.formationds.commons.model.capacity.builder.CapacityDeDupRatioBuilder}
   */
  public CapacityDeDupRatioBuilder withRatio( Double ratio ) {
    this.ratio = ratio;
    return this;
  }

  /**
   * @param volumeName the {@link String} representing the volume name
   *
   * @return Returns the {@link com.formationds.commons.model.capacity.builder.CapacityDeDupRatioBuilder}
   */
  public CapacityDeDupRatioBuilder withVolumeName( String volumeName ) {
    this.volumeName = volumeName;
    return this;
  }

  /**
   * @return Returns the {@link CapacityDeDupRatio}
   */
  public CapacityDeDupRatio build() {
    return new CapacityDeDupRatio( volumeName, ratio );
  }
}
