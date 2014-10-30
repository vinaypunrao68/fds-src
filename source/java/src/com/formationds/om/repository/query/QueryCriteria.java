/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query;

import com.formationds.commons.crud.SearchCriteria;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.Metrics;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
@SuppressWarnings("UnusedDeclaration")
public class QueryCriteria
    extends ModelBase
    implements SearchCriteria {
    private static final long serialVersionUID = -5554442148762258901L;

    private DateRange range;           // date ranger ; starting and ending
    private Integer points;            // number of points to provide in results
    private Long firstPoint;           // first point, i.e. row number
    // TODO implement Interval         // the interval MINUTE, HOUR, DAY, WEEK, etc.
    private List<Metrics> seriesType;  // the series type ( a list of Metrics )
    private List<Context> contexts;    // the context
    private Boolean tenantAdmin;
    private Boolean admin;

    /**
     * default constructor
     */
    public QueryCriteria() {
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
    public List<Metrics> getSeriesType() {
        if( seriesType == null ) {
            this.seriesType = new ArrayList<>();
        }
        return seriesType;
    }

    /**
     * @param seriesType the {@link List} of {@link com.formationds.commons.model.type.Metrics}
     */
    public void setSeriesType( final List<Metrics> seriesType ) {
        this.seriesType = seriesType;
    }

    /**
     * @return Returns the {@link List} of {@link com.formationds.commons.model.abs.Context}
     */
    public List<Context> getContexts() {
        if( contexts == null ) {
            this.contexts = new ArrayList<>();
        }
        return contexts;
    }

    /**
     * @param contexts the {@link List} of {@link com.formationds.commons.model.abs.Context}
     */
    public void setContexts( final List<Context> contexts ) {
        this.contexts = contexts;
    }

    /**
     * @return Returns the {@link Integer} representing the maximum number of
     * entity to return
     */
    public Integer getPoints() {
        return points;
    }

    /**
     * @param points the {@link Integer} representing the maximum number of
     *               entity to return
     */
    public void setPoints( final Integer points ) {
        this.points = points;
    }

    /**
     * @return Returns the {@link Long} representing the first results
     */
    public Long getFirstPoint() {
        return firstPoint;
    }

    /**
     * @param firstPoint the {@link Long} representing the first results
     */
    public void setFirstPoint( final Long firstPoint ) {
        this.firstPoint = firstPoint;
    }

    /**
     * @return Returns {@code boolean} if the query is for tenant admin
     */
    public Boolean isTenantAdmin() {
        return tenantAdmin;
    }

    /**
     * @param tenantAdmin the {@code boolean} representing the tenant admin
     */
    public void setTenantAdmin( final Boolean tenantAdmin ) {
        this.tenantAdmin = tenantAdmin;
    }

    /**
     * @return Returns {@code boolean} if the query is for platform admin
     */
    public Boolean isAdmin() {
        return admin;
    }

    /**
     * @param admin the {@code boolean} representing the platform admin
     */
    public void setAdmin( final Boolean admin ) {
        this.admin = admin;
    }
}
