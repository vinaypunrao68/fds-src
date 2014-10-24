/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity;

import com.formationds.commons.crud.SearchCriteria;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.Metrics;

import javax.persistence.NamedQueries;
import javax.persistence.NamedQuery;
import java.util.List;

/**
 * @author davefds
 */
@NamedQueries(
  {
    @NamedQuery(name = "Query.Events",
                query = "")
  }
)
public class EventQuery extends ModelBase implements SearchCriteria {
  private static final long serialVersionUID = 1;

  private DateRange range;
  private List<Context> contexts;    // the context

  /**
   * default constructor
   */
  public EventQuery() {
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.DateRange}
   */
  public DateRange getRange() {
    return range;
  }

  /**
   * @param range the {@link com.formationds.commons.model.DateRange}
   */
  public void setRange( final DateRange range ) {
    this.range = range;
  }

  /**
   * @return Returns the {@link java.util.List} of {@link Context}
   */
  public List<Context> getContexts() {
    return contexts;
  }

  /**
   * @param contexts the {@link java.util.List} of {@link Context}
   */
  public void setContexts( final List<Context> contexts ) {
    this.contexts = contexts;
  }
}
