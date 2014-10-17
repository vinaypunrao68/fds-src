/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import java.util.concurrent.TimeUnit;

/**
 * @author ptinius
 */
public enum CollectionInterval {
  DAY( TimeUnit.MINUTES, 30L,
       TimeUnit.MINUTES, 5L ),
  WEEK( TimeUnit.DAYS, 7L,
        TimeUnit.MINUTES, 30L ),
  MONTH( TimeUnit.DAYS, 31L,
         TimeUnit.HOURS, 2L ),
  YEAR( TimeUnit.DAYS, 366L,
        TimeUnit.DAYS, 1L );

  private final TimeUnit intervalUnit;
  private final Long interval;
  private final TimeUnit frequencyUnit;
  private final Long frequency;

  CollectionInterval( final TimeUnit intervalUnit,
                      final Long interval,
                      final TimeUnit frequencyUnit,
                      final Long frequency ) {
    this.intervalUnit = intervalUnit;
    this.interval = interval;
    this.frequency = frequency;
    this.frequencyUnit = frequencyUnit;
  }

  /**
   * @return Returns the {@code interval} represented in {@code intervalUnit}
   * in seconds
   */
  public Long getInterval() {
    return intervalUnit.toSeconds( interval );
  }

  /**
   * @return Returns the {@code frequency} represented in {@code frequencyUnit}
   * in seconds
   */
  public Long getFrequency() {
    return frequencyUnit.toSeconds( frequency );
  }
}
