/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import org.junit.Test;

public class CollectionIntervalTest {
  private static final String FORMAT = "%5s %10d %10d\n";

  @Test
  public void test()
    throws Exception {

    for( final CollectionInterval x : CollectionInterval.values() ) {
      System.out.printf( FORMAT,
                         x.name(),
                         x.getInterval(),
                         x.getFrequency() );
    }
  }
}