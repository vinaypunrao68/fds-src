/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.query.MetricQueryCriteria;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class MetricQueryCriteriaBuilder {

    // TODO implement Interval         // the interval MINUTE, HOUR, DAY, WEEK, etc.
    private List<Metrics> seriesType;  // the series type ( a list of Metrics )
    private Boolean tenantAdmin;
    private Boolean admin;
    private DateRange range;           // date range ; starting and ending
    private Integer points;            // number of points to provide in results
    private Long firstPoint;           // first point, i.e. row number
    private List<Context> contexts;    // the context

    /**
     * default constructor
     */
    public MetricQueryCriteriaBuilder( ) {
    }

    /**
     * @param seriesTypes the {@link List} of {@link com.formationds.commons.model.type.Metrics}
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withSeriesTypes( List<Metrics> seriesTypes ) {
        this.seriesType = seriesTypes;
        return this;
    }

    /**
     * @param seriesType the {@link com.formationds.commons.model.type.Metrics}
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withSeriesType( Metrics seriesType ) {
        if( this.seriesType == null ) {
            this.seriesType = new ArrayList<>( );
        }

        this.seriesType.add( seriesType );
        return this;
    }

    /**
     * @param tenantAdmin the {@code boolean} representing the tenant admin
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withTenantAdmin( Boolean tenantAdmin ) {
        this.tenantAdmin = tenantAdmin;
        return this;
    }

    /**
     * @param admin the {@code boolean} representing the system admin
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withAdmin( Boolean admin ) {
        this.admin = admin;
        return this;
    }

    /**
     * @param range the {@link com.formationds.commons.model.DateRange}
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withRange( DateRange range ) {
        this.range = range;
        return this;
    }

    /**
     * @param points the {@link Integer} representing the points
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withPoints( Integer points ) {
        this.points = points;
        return this;
    }

    /**
     * @param firstPoint the {@link Long} representing paging starting point
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withFirstPoint( Long firstPoint ) {
        this.firstPoint = firstPoint;
        return this;
    }

    /**
     * @param contexts the {@link com.formationds.commons.model.abs.Context}
     *
     * @return Returns the {@link com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder}
     */
    public MetricQueryCriteriaBuilder withContexts( List<Context> contexts ) {
        this.contexts = contexts;
        return this;
    }

    /**
     * @return Returns the {@link MetricQueryCriteria}
     */
    public MetricQueryCriteria build( ) {
        MetricQueryCriteria metricQueryCriteria = new MetricQueryCriteria();
        metricQueryCriteria.setSeriesType( seriesType );
        metricQueryCriteria.setTenantAdmin( tenantAdmin );
        metricQueryCriteria.setAdmin( admin );
        metricQueryCriteria.setRange( range );
        metricQueryCriteria.setPoints( points );
        metricQueryCriteria.setFirstPoint( firstPoint );
        metricQueryCriteria.setContexts( contexts );
        return metricQueryCriteria;
    }
}
