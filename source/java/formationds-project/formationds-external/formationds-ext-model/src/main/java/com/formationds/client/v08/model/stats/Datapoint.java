/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.stats;


/**
 * @author ptinius
 */
public class Datapoint {
  @SuppressWarnings("unused")
private static final long serialVersionUID = 7313793869030172350L;

  private Double x;         // x-axis data point
  private Double y;         // y-axis time point
  private Boolean isReal = true;  // this field can help us determine whether the point is observed, or filler.

  public Datapoint(){}
  
  /**
   * @return Returns the {@link Long} representing the x-axis
   */
  public Double getX() {
    return x;
  }

  /**
   * @param x the {@link Long} representing the x-axis
   */
  public void setX( final Double x ) {
    this.x = x;
  }

  /**
   * @return Returns the {@link Long} representing the y-axis
   */
  public Double getY() {
    return y;
  }

  /**
   * @param y the {@link Long} representing the y-axis
   */
  public void setY( final Double y ) {
    this.y = y;
  }
  
  /**
   * 
   * @return Returns the {@link Boolean} representing whether this point is observed or not.
   */
  public Boolean isReal(){
	  return this.isReal;
  }
  
  /**
   * 
   * @param isIt the {@link Boolean} representing the authenticity of this data point
   */
  public void setIsReal( Boolean isIt ){
	  this.isReal = isIt;
  }

  /**
   * @return a concise string representation of the datapoint
   */
  public String toString() { return String.format("Datapoint (%f, %f)", x, y); }
}
