/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import java.util.Date;

/**
 * @author ptinius
 */
public class DateRange
  extends ModelBase {
  private static final long serialVersionUID = -7728219218469818163L;

  private Long start;
  private Long end;

  /**
   * @return the {@link Long} representing the starting date in milliseconds
   */
  public Long getStart() {
    return start;
  }

  /**
   * @param start Returns the {@link Date} representing the starting date in milliseconds
   */
  public void setStart( final Long start ) {
    this.start = start;
  }

  /**
   * @return the {@link Long} representing the ending date in milliseconds
   */
  public Long getEnd() {
    return end;
  }

  /**
   * @param end Returns the {@link Long} representing the ending date in milliseconds
   */
  public void setEnd( final Long end ) {
    this.end = end;
  }
}
