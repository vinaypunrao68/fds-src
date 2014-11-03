/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query;

import com.formationds.commons.crud.SearchCriteria;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.abs.ModelBase;

import java.util.ArrayList;
import java.util.List;

public class QueryCriteria extends ModelBase implements SearchCriteria {
    private DateRange range;           // date range ; starting and ending
    private Integer points;            // number of points to provide in results
    private Long firstPoint;           // first point, i.e. row number
    private List<Context> contexts;    // the context

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
     * @return Returns the {@link java.util.List} of {@link com.formationds.commons.model.abs.Context}
     */
    public List<Context> getContexts() {
        if( contexts == null ) {
            this.contexts = new ArrayList<>();
        }
        return contexts;
    }

    /**
     * @param contexts the {@link java.util.List} of {@link com.formationds.commons.model.abs.Context}
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
}
