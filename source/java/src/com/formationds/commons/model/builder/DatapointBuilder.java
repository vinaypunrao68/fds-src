/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Datapoint;

/**
 * @author ptinius
 */
public class DatapointBuilder {
  private Double x;         // x-axis data point
  private Double y;         // y-axis time point

  /**
   * default constructor
   */
  public DatapointBuilder() {
  }

  /**
   * @param x the {@link Long} representing the x-axis
   *
   * @return Returns {@link com.formationds.commons.model.builder.DatapointBuilder}
   */
  public DatapointBuilder withX( Double x ) {
    this.x = x;
    return this;
  }

  /**
   * @param y the {@link Long} representing the y-axis
   *
   * @return Returns {@link com.formationds.commons.model.builder.DatapointBuilder}
   */
  public DatapointBuilder withY( Double y ) {
    this.y = y;
    return this;
  }

  /**
   * @return Returns {@link com.formationds.commons.model.Datapoint}
   */
  public Datapoint build() {
    Datapoint datapoint = new Datapoint();

    if( x != null ) {
      datapoint.setX( x );
    }

    if( y != null ) {
      datapoint.setY( y );
    }

    return datapoint;
  }
}
