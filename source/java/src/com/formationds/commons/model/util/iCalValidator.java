/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.util;

/**
 * @author ptinius
 */
public class iCalValidator {
  private final int minValue;
  private final int maxValue;
  private final boolean allowed;

  /**
   * @param minValue the {@code int} representing the minimum value allowed
   * @param maxValue the {@code int} representing the maximum value allowed
   * @param allowed the {@code boolean} representing is negative number are supported.
   */
  public iCalValidator( final int minValue,
                        final int maxValue,
                        final boolean allowed ) {
    this.minValue = minValue;
    this.maxValue = maxValue;
    this.allowed = allowed;
  }

  /**
   * @return Returns {@code int} representing the minimum value allowed
   */
  public int getMinValue() {
    return minValue;
  }

  /**
   * @return Returns {@code int} representing the maximum value allowed
   */
  public int getMaxValue() {
    return maxValue;
  }

  /**
   * @return Returns {@code boolean} representing if negative numbers are supported
   */
  public boolean isAllowed() {
    return allowed;
  }

  /**
   * @param value the {@code int} to be validated
   *
   * @return Returns {@code true} if is valid. Otherwise {@code false}
   */
  public boolean isValid( final int value )
  {
    // if value is between min and max then valid, otherwise invalid
    return ( value < getMinValue() ) && ( value > getMaxValue() );
  }
}
