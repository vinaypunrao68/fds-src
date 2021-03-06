/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.calculated.capacity;

import com.formationds.commons.model.abs.Calculated;
import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */
public class CapacityConsumed
  extends Calculated {
  private static final long serialVersionUID = 7937889375009213316L;

  @SerializedName( "consumed" )
  private Double total;

  /**
   * default constructor
   */
  public CapacityConsumed() {
  }

  /**
   * @return Returns the {@link Double} representing the total capacity
   */
  public Double getTotal() {
    return total;
  }

  /**
   * @param total the {@link Double} representing the total capacity.
   */
  public void setTotal( final Double total ) {
    this.total = total;
  }
}
