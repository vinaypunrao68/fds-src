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

  private Date start;
  private Date end;

  /**
   * @return the {@link Date} representing the starting date
   */
  public Date getStart() {
    return start;
  }

  /**
   * @param start Returns the {@link Date} representing the starting date
   */
  public void setStart( final Date start ) {
    this.start = start;
  }

  /**
   * @return the {@link Date} representing the ending date
   */
  public Date getEnd() {
    return end;
  }

  /**
   * @param end Returns the {@link Date} representing the ending date
   */
  public void setEnd( final Date end ) {
    this.end = end;
  }
}
