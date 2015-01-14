/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.calculated.capacity;

import com.formationds.commons.model.abs.Calculated;
import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */
public class CapacityToFull
  extends Calculated {
  private static final long serialVersionUID = 4144617532432238800L;

  @SerializedName( "to_full" )
  // value is in seconds
  private Long toFull;

  /**
   * default constructor
   */
  public CapacityToFull() {
  }

  /**
   * @return Returns the {@link java.lang.Integer} representing the duration
   * until capacity is 100% (full)
   */
  public Long getToFull() {
    return toFull;
  }

  /**
   * @param toFull the {@link java.lang.Integer}  representing the duration
   *               until capacity is 100% (full)
   */
  public void setToFull( final Long toFull ) {
    this.toFull = toFull;
  }
}
