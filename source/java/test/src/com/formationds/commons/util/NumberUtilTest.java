/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import org.junit.Test;

public class NumberUtilTest {

  @Test
  public void test() {
    for( final Integer integer : NumbersUtil.toNumbersArray( "0,20,40", "," ) ) {
      System.out.println( integer );
    }
  }
}