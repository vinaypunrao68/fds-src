/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.capacity;

import com.formationds.commons.model.abs.Calculated;

/**
 * @author ptinius
 */
public class CapacityConsumed
  extends Calculated {
  private static final long serialVersionUID = 7937889375009213316L;

  private Double total;

  /**
   * default constructor
   */
  public CapacityConsumed() {
  }

  /**
   * @param total the {@link java.lang.Double} representing the total capacity
   */
  public CapacityConsumed( final Double total ) {
    this.total = total;
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
