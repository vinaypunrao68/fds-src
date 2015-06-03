/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.util;

/**
 * @author ptinius
 */
public class FDSStringUtil {

  /**
   * @param str the {@link String} to upper case first letters
   *
   * @return Returns {@link String} the upper cased first letters
   */
  public static String UppercaseFirstLetters( String str ) {
    boolean prevWasWhiteSp = true;
    char[] chars = str.toCharArray();
    for( int i = 0;
         i < chars.length;
         i++ ) {
      if( Character.isLetter( chars[ i ] ) ) {
        if( prevWasWhiteSp ) {
          chars[ i ] = Character.toUpperCase( chars[ i ] );
        }
        prevWasWhiteSp = false;
      } else {
        prevWasWhiteSp = Character.isWhitespace( chars[ i ] );
      }
    }
    return new String( chars );
  }
}
