/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class DateRange extends ModelBase {
    private static final long serialVersionUID = -7728219218469818163L;

  	// TIMES ARE IN SECONDS WHEN USED WITH STATS QUERY
    private Long start;
    private Long end;

    public DateRange() {}

    /**
     * @param start
     */
    public DateRange(Long start) {
      this.start = start;
    }

    /**
     *
     * @param start
     * @param end
     */
    public DateRange(Long start, Long end) {
      this.start = start;
      this.end = end;
    }

    /**
     * @return the {@link Long} representing the starting timestamp
     */
    public Long getStart() {
        return start;
  }

    /**
     * If using with the stats query API please use seconds
     * 
    * @param start Returns the {@link Long} representing the starting timestamp
    */
    public void setStart( final Long start ) {
      this.start = start;
    }

    /**
    * @return the {@link Long} representing the ending timestamp
    */
    public Long getEnd() {
      return end;
    }

    /**
     * If using with the stats api please use seconds
     * 
    * @param end Returns the {@link Long} representing the endind timestamp
    */
    public void setEnd( final Long end ) {
      this.end = end;
    }
}
