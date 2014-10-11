/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.ResultType;

import javax.persistence.NamedQueries;
import javax.persistence.NamedQuery;
import java.util.List;

/**
 * @author ptinius
 */
@NamedQueries(
  {
    @NamedQuery( name="Query.Average",
               query="" ),
    @NamedQuery( name="Query.Sum",
                 query="" )
  }
)
public class MetricQuery
  extends ModelBase {
  private static final long serialVersionUID = -5554442148762258901L;

  private DateRange range;
  // TODO implement points;          // number of points to provide in results
  // TODO implement Interval         // the interval MINUTE, HOUR, DAY, WEEK, etc.
  private Metrics seriesType;        // the series type ( a list of Metrics )
  private List<Context> contexts;    // the context
  private ResultType resultType;

  /**
   * default constructor
   */
  public MetricQuery() {
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
   * @return Returns the {@link com.formationds.commons.model.type.Metrics}
   */
  public Metrics getSeriesType() {
    return seriesType;
  }

  /**
   * @param seriesType the {@link com.formationds.commons.model.type.Metrics}
   */
  public void setSeriesType( final Metrics seriesType ) {
    this.seriesType = seriesType;
  }

  /**
   * @return Returns the {@link List} of {@link com.formationds.commons.model.Context}
   */
  public List<Context> getContexts() {
    return contexts;
  }

  /**
   * @param contexts the {@link List} of {@link com.formationds.commons.model.Context}
   */
  public void setContexts( final List<Context> contexts ) {
    this.contexts = contexts;
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.type.ResultType}
   */
  public ResultType getResultType() {
    return resultType;
  }

  /**
   * @param resultType the {@link com.formationds.commons.model.type.ResultType}
   */
  public void setResultType( final ResultType resultType ) {
    this.resultType = resultType;
  }
}
