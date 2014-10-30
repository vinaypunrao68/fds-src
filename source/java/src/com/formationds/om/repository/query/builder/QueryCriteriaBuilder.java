/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;

import java.util.ArrayList;
import java.util.List;

public class QueryCriteriaBuilder {
    protected DateRange range;           // date range ; starting and ending
    protected Integer points;            // number of points to provide in results
    protected Long firstPoint;           // first point, i.e. row number
    protected List<Context> contexts;    // the context

    public QueryCriteriaBuilder withRange(DateRange range) {
        this.range = range;
        return this;
    }

    public QueryCriteriaBuilder withPoints(Integer points) {
        this.points = points;
        return this;
    }

    public QueryCriteriaBuilder withFirstPoint(Long firstPoint) {
        this.firstPoint = firstPoint;
        return this;
    }

    public QueryCriteriaBuilder withContexts(Context context) {
        if (this.contexts == null) {
            this.contexts = new ArrayList<>();
        }
        this.contexts.add(context);
        return this;
    }
}
