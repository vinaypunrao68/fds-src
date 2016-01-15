/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query;

import com.formationds.commons.model.type.Metrics;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

/**
 * @author ptinius
 */
@SuppressWarnings("UnusedDeclaration")
public class MetricQueryCriteria
        extends QueryCriteria {
    private static final long serialVersionUID = -5554442148762258901L;

    // TODO implement Interval         // the interval MINUTE, HOUR, DAY, WEEK, etc.
    private List<Metrics> seriesType;  // the series type ( a list of Metrics )
    private Boolean tenantAdmin;
    private Boolean admin;

    /**
     * default constructor
     */
    public MetricQueryCriteria(QueryType type) {
    	super.setQueryType(type);
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
     * @param seriesType the {@link com.formationds.commons.model.type.Metrics}
     */
    public void setSeriesType( final Metrics seriesType ) {
        if( this.seriesType == null ) {
            this.seriesType = new ArrayList<>();
        }

        this.seriesType.add( seriesType );

        super.addColumns( seriesType.name() );
    }

    /**
     * @param seriesType the {@link List} of {@link com.formationds.commons.model.type.Metrics}
     */
    public void setSeriesType( final List<Metrics> seriesType ) {
        this.seriesType = seriesType;
        List<String> columnNames = seriesType.stream().map( Enum::name ).collect( Collectors.toList() );
        super.setColumns( columnNames );
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
