/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.calculation;

/**
 * @author ptinius
 */
public class Calculation {
  /*
   * current thinking is the initial threshold will be set to 3. To come up
   * with the better threshold more investigation will be required.
   */
  private static final Double THRESHOLD = 3.0;

  /**
   * @param shortTerm the {@link Double} representing the short term sigma
   * @param longTerm  the {@link Double} representing the long term sigma
   *
   * @return Returns {@code true} if firebreak has occurred. Otherwise, {@code
   * false} is returned.
   */
  public static boolean isFirebreak( final Double shortTerm,
                                     final Double longTerm ) {
    return ( ( shortTerm / longTerm ) > THRESHOLD );
  }

  /**
   * @param enumerator  the {@link Double} representing the enumerator
   * @param denominator the {@link Double} representing the denominator
   *
   * @return Returns {@link Double} representing the ratio
   */
  public static Double ratio( final double enumerator,
                              final Double denominator ) {
    if( ( enumerator > 0.0 ) && ( denominator > 0.0 ) ) {
      return enumerator / denominator;
    }

    return 0.0;
  }
}
