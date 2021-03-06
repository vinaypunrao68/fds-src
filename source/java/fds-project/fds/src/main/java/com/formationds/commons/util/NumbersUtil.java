/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import org.apache.commons.lang3.StringUtils;

import java.text.DecimalFormatSymbols;

/**
 * @author ptinius
 */
public class NumbersUtil {
  /**
   * @param string    the {@link String} representing the numbers, delimited by
   *                  {@code delimited}
   * @param delimiter the {@link String} representing the delimiter
   *
   * @return Returns {@code array} of {@link Integer}
   */
  public static Integer[] toNumbersArray( final String string,
                                          final String delimiter ) {
    final Numbers<Integer> ints = new Numbers<>();
    final String[] split = string.split( delimiter );
    for( final String aSplit : split ) {
      ints.add( Integer.valueOf( aSplit ) );
    }

    return ints.toArray( new Integer[ ints.size() ] );
  }

  /**
   * @param string    the {@link String} representing the numbers, delimited by
   *                  {@code delimited}
   * @param delimiter the {@link String} representing the delimiter
   *
   * @return Returns {@link java.util.List} of {@link java.lang.Integer}
   */
  public static Numbers<Integer> toNumbersList( final String string,
                                                final String delimiter ) {
    final Numbers<Integer> ints = new Numbers<>();
    final String[] split = string.split( delimiter );
    for( final String aSplit : split ) {
      ints.add( Integer.valueOf( aSplit ) );
    }

    return ints;
  }

  /**
   * @param numbers   the {@link Numbers}
   * @param separator the {@link String} separator
   *
   * @return Returns {@link String} contain {@code numbers} separated by {@code
   * separator}
   */
  public static String toStringFromIntegerListSeparatedBy(
    final Numbers<Integer> numbers,
    final String separator ) {
    return StringUtils.join( numbers.toArray( new Integer[ numbers.size() ] ),
                             separator );
  }

  /**
   * @param numbers   the {@link Numbers}
   * @param separator the {@link String} separator
   *
   * @return Returns {@link String} contain {@code numbers} separated by {@code
   * separator}
   */
  public static String toStringFromLongListListSeparatedBy(
    final Numbers<Long> numbers,
    final String separator ) {
    return StringUtils.join( numbers.toArray( new Long[ numbers.size() ] ),
                             separator );
  }

  public static boolean isStringNumeric( String str ) {

    final DecimalFormatSymbols currentLocaleSymbols =
        DecimalFormatSymbols.getInstance();
    final char localeMinusSign = currentLocaleSymbols.getMinusSign();

    if( !Character.isDigit( str.charAt( 0 ) ) &&
        str.charAt( 0 ) != localeMinusSign ) {

      return false;

    }

    boolean isDecimalSeparatorFound = false;
    final char localeDecimalSeparator =
        currentLocaleSymbols.getDecimalSeparator();

    for( char c : str.substring( 1 )
                     .toCharArray() ) {

      if( !Character.isDigit( c ) ) {

        if( c == localeDecimalSeparator && !isDecimalSeparatorFound ) {

          isDecimalSeparatorFound = true;
          continue;

        }

        return false;

      }
    }

    return true;
  }
}
