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
  private Integer toFull;

  /**
   * default constructor
   */
  public CapacityToFull() {
  }

  /**
   * @param toFull the {@link java.lang.Integer} representing the duration
   *               until capacity is 100% (full).
   */
  public CapacityToFull( final Integer toFull ) {
    this.toFull = toFull;
  }

  /**
   * @return Returns the {@link java.lang.Integer}  representing the duration
   * until capacity is 100% (full)
   */
  public Integer getToFull() {
    return toFull;
  }

  /**
   * @param toFull the {@link java.lang.Integer}  representing the duration
   *               until capacity is 100% (full)
   */
  public void setToFull( final Integer toFull ) {
    this.toFull = toFull;
  }
}
