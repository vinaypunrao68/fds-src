/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.DateRange;

import java.util.Date;

/**
 * @author ptinius
 */
public class DateRangeBuilder {
  private Date start;
  private Date end;

  /**
   * default constructor
   */
  public DateRangeBuilder() {
  }

  /**
   * @param start the {@link Date} representing the starting date.
   * @param end the {@link Date} representing the ending date.
   */
  public DateRangeBuilder( final Date start, final Date end ) {
    withStart( start );
    withEnd( end );
  }

  /**
   * @param start the {@link Date} representing the start date
   *
   * @return Returns {@link com.formationds.commons.model.builder.DateRangeBuilder}
   */
  public DateRangeBuilder withStart( Date start ) {
    this.start = start;
    return this;
  }

  /**
   * @param end the {@link Date} representing the end date
   *
   * @return Returns {@link com.formationds.commons.model.builder.DateRangeBuilder}
   */
  public DateRangeBuilder withEnd( Date end ) {
    this.end = end;
    return this;
  }

  /**
   * @return Returns {@link com.formationds.commons.model.DateRange}
   */
  public DateRange build() {
    DateRange dateRange = new DateRange();
    dateRange.setStart( start );
    dateRange.setEnd( end );
    return dateRange;
  }
}
