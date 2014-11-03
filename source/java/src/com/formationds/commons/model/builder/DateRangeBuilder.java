/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.DateRange;

import java.sql.Timestamp;

/**
 * @author ptinius
 */
public class DateRangeBuilder {
  private Timestamp start;
  private Timestamp end;

  /**
   * default constructor
   */
  public DateRangeBuilder() {
  }

  /**
   * @param start the {@link Timestamp} representing the start timestamp.
   * @param end the {@link Timestamp} representing the end timestamp.
   */
  public DateRangeBuilder( final Timestamp start, final Timestamp end ) {
    withStart( start );
    withEnd( end );
  }

  /**
   * @param start the {@link Timestamp} representing the start timestamp
   *
   * @return Returns {@link com.formationds.commons.model.builder.DateRangeBuilder}
   */
  public DateRangeBuilder withStart( Timestamp start ) {
    this.start = start;
    return this;
  }

  /**
   * @param end the {@link Timestamp} representing the end timestamp
   *
   * @return Returns {@link com.formationds.commons.model.builder.DateRangeBuilder}
   */
  public DateRangeBuilder withEnd( Timestamp end ) {
    this.end = end;
    return this;
  }

  /**
   * @return Returns {@link com.formationds.commons.model.DateRange}
   */
  public DateRange build() {
    if( start == null && end == null ) {
      throw new IllegalArgumentException( );
    }

    DateRange dateRange = new DateRange();
    if( start != null ) {
      dateRange.setStart( start.getTime() );
    }

    if( end != null ) {
      dateRange.setEnd( end.getTime() );
    }

    return dateRange;
  }
}
