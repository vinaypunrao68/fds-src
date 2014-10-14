/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.util.concurrent.TimeUnit;

/**
 * @author ptinius
 */
public class ElapseTimer {
  long starts;

  /**
   * @return Returns an instance of {@link com.formationds.commons.util.ElapseTimer}
   */
  public static ElapseTimer start() {
    return new ElapseTimer();
  }

  /**
   * default private constructor
   */
  private ElapseTimer() {
    reset();
  }

  /**
   * @return Returns the reset {@link com.formationds.commons.util.ElapseTimer}
   */
  public ElapseTimer reset() {
    starts = System.currentTimeMillis();
    return this;
  }

  /**
   * @return Returns {@code long} representing the elapse time
   */
  public long time() {
    long ends = System.currentTimeMillis();
    return ends - starts;
  }

  /**
   * @param unit the {@link java.util.concurrent.TimeUnit}
   *
   * @return Returns {@code long} representing the elapse time
   */
  public long time( TimeUnit unit ) {
    return unit.convert( time(), TimeUnit.MILLISECONDS );
  }
}
