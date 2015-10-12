/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.stats.query;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */

public class MetricQueryCriteria extends QueryCriteria {
    
	public static final String AVG_TRANSACTIONS = "average";
	public static final String PERCENTAGE = "percentage";
	
	@SuppressWarnings("unused")
	private static final long serialVersionUID = -5554442148762258901L;

    private List<String> seriesType;  // the series type ( a list of Metrics )

    /**
     * default constructor
     */
    public MetricQueryCriteria() {
    }

    /**
     * @return Returns the {@link com.formationds.commons.model.type.Metrics}
     */
    public List<String> getSeriesType() {
        if( seriesType == null ) {
            this.seriesType = new ArrayList<>();
        }
        return seriesType;
    }

    /**
     * @param seriesType the {@link com.formationds.commons.model.type.Metrics}
     */
    public void setSeriesType( final String seriesType ) {
        if( this.seriesType == null ) {
            this.seriesType = new ArrayList<>();
        }

        this.seriesType.add( seriesType );

        super.addColumns( seriesType );
    }

    /**
     * @param seriesType the {@link List} of {@link com.formationds.commons.model.type.Metrics}
     */
    public void setSeriesType( final List<String> seriesType ) {
        this.seriesType = seriesType;
        super.setColumns( seriesType );
    }
}
