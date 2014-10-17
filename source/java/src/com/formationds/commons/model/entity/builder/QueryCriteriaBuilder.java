/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity.builder;

import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.entity.QueryCriteria;
import com.formationds.commons.model.type.Metrics;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class QueryCriteriaBuilder {
  private DateRange range;           // date ranger ; starting and ending
  private Integer points;            // number of points to provide in results
  private Long firstPoint;           // first point, i.e. row number
  // TODO implement Interval         // the interval MINUTE, HOUR, DAY, WEEK, etc.
  private List<Metrics> seriesType;  // the series type ( a list of Metrics )
  private List<Context> contexts;    // the context

  public QueryCriteriaBuilder() {
  }

  public QueryCriteriaBuilder withRange( DateRange range ) {
    this.range = range;
    return this;
  }

  public QueryCriteriaBuilder withPoints( Integer points ) {
    this.points = points;
    return this;
  }

  public QueryCriteriaBuilder withFirstPoint( Long firstPoint ) {
    this.firstPoint = firstPoint;
    return this;
  }

  public QueryCriteriaBuilder withSeriesType( Metrics seriesType ) {
    if( this.seriesType == null ) {
      this.seriesType = new ArrayList<>();
    }

    this.seriesType.add( seriesType );
    return this;
  }

  public QueryCriteriaBuilder withContexts( Context context ) {
    if( this.contexts == null ) {
      this.contexts = new ArrayList<>();
    }
    this.contexts.add( context );
    return this;
  }

  public QueryCriteria build() {

    QueryCriteria queryCriteria = new QueryCriteria();

    if( range != null ) {
      queryCriteria.setRange( range );
    }
    if( points != null ) {
      queryCriteria.setPoints( points );
    }
    if( firstPoint != null ) {
      queryCriteria.setFirstPoint( firstPoint );
    }
    queryCriteria.setSeriesType( seriesType );
    queryCriteria.setContexts( contexts );

    return queryCriteria;
  }
}
