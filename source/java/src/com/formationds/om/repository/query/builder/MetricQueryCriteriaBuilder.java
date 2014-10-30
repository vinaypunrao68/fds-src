/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.QueryCriteria;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class MetricQueryCriteriaBuilder extends QueryCriteriaBuilder {
    // TODO implement Interval         // the interval MINUTE, HOUR, DAY, WEEK, etc.
  private List<Metrics> seriesType;  // the series type ( a list of Metrics )

    public MetricQueryCriteriaBuilder() {
  }

    public QueryCriteriaBuilder withSeriesType( Metrics seriesType ) {
    if( this.seriesType == null ) {
      this.seriesType = new ArrayList<>();
    }

    this.seriesType.add( seriesType );
    return this;
  }

    public QueryCriteria build() {

    MetricQueryCriteria queryCriteria = new MetricQueryCriteria();

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
