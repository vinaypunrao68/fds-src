/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class Datapoint
  extends ModelBase {
  private static final long serialVersionUID = 7313793869030172350L;

  private Long x;         // x-axis data point
  private Long y;         // y-axis time point

  /**
   * @return Returns the {@link Long} representing the x-axis
   */
  public Long getX() {
    return x;
  }

  /**
   * @param x the {@link Long} representing the x-axis
   */
  public void setX( final Long x ) {
    this.x = x;
  }

  /**
   * @return Returns the {@link Long} representing the y-axis
   */
  public Long getY() {
    return y;
  }

  /**
   * @param y the {@link Long} representing the y-axis
   */
  public void setY( final Long y ) {
    this.y = y;
  }
}
