/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Context;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.MetricQuery;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.ResultType;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class MetricQueryBuilder {

  private DateRange range;
  // TODO implement points;         // number of points to provide in results
  // TODO implement Interval        // the interval MINUTE, HOUR, DAY, WEEK, etc.
  private Metrics seriesType;       // the series type ( Metrics.key() )
  private List<Context> contexts;   // the context for the search
  private ResultType resultType;

  /**
   * default constructor
   */
  public MetricQueryBuilder() {
  }

  /**
   * @param range the {@link com.formationds.commons.model.DateRange}
   *
   * @return Returns {@link MetricQueryBuilder}
   */
  public MetricQueryBuilder withRange( DateRange range ) {
    this.range = range;
    return this;
  }

  /**
   * @param contexts the {@link List} of {@link Context}
   *
   * @return Returns {@link MetricQueryBuilder}
   */
  public MetricQueryBuilder withContexts( List<Context> contexts ) {
    this.contexts = contexts;
    return this;
  }

  /**
   * @param context the {@link Context}
   *
   * @return Returns {@link MetricQueryBuilder}
   */
  public MetricQueryBuilder withContext( Context context ) {
    if( this.contexts == null ) {
      this.contexts = new ArrayList<>();
    }

    this.contexts.add( context );
    return this;
  }

  /**
   * @param resultType the {@link com.formationds.commons.model.type.ResultType}
   *
   * @return Returns {@link MetricQueryBuilder}
   */
  public MetricQueryBuilder withResultType( ResultType resultType ) {
    this.resultType = resultType;
    return this;
  }

  /**
   * @param resultType the {@link com.formationds.commons.model.type.ResultType}
   *
   * @return Returns {@link MetricQueryBuilder}
   */
  public MetricQueryBuilder withResultType( String resultType ) {
    this.resultType = ResultType.valueOf( resultType );
    return this;
  }

  /**
   * @param seriesType the {@link com.formationds.commons.model.type.Metrics}
   *
   * @return Return {@link MetricQueryBuilder}
   */
  public MetricQueryBuilder withSeriesType( Metrics seriesType ) {
    this.seriesType = seriesType;
    return this;
  }

  /**
   * @param seriesType the {@link String} representing the {@link
   *                   com.formationds.commons.model.type.Metrics}
   *
   * @return Return {@link MetricQueryBuilder}
   */
  public MetricQueryBuilder withSeriesType( String seriesType ) {
    this.seriesType = Metrics.valueOf( seriesType );
    return this;
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.MetricQuery}
   */
  public MetricQuery build() {
    MetricQuery metricQuery = new MetricQuery();

    if( range != null ) {
      metricQuery.setRange( range );
    }

    if( seriesType != null ) {
      metricQuery.setSeriesType( seriesType );
    }

    if( contexts != null ) {
      metricQuery.setContexts( contexts );
    }

    if( resultType != null ) {
      metricQuery.setResultType( resultType );
    }

    return metricQuery;
  }
}
