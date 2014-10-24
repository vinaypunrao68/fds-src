/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

/**
 * @author ptinius
 */
public class DateTimeUtil {

  /**
   * @param year the {@link Integer} representing the year
   *
   * @return Returns {@code true} if {@code year} is a leap tear. Otherwise,
   * {@code false} is returned
   */
  public static boolean isLeapYear( final Integer year ) {
    return ( year % 4 == 0 ) && ( ( year % 100 != 0 ) || ( year % 400 == 0 ) );
  }

  /**
   * @param year the {@link Integer} representing the year
   *
   * @return Returns {@link Integer} representing the length of {@code year}
   */
  public static int yearLength( final Integer year ) {
    return isLeapYear( year ) ? 366 : 365;
  }

  /**
   * @param epoch the {@code long} representing number of seconds since January 1, 1970.
   *
   * @return Returns {@code long} representing
   */
  public static Long epochToMilliseconds( final long epoch ) {
    return epoch * 1000;
  }
}
