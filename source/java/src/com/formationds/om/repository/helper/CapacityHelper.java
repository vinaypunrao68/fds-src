/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.model.Series;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.query.QueryCriteria;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class CapacityHelper {

    private static final Logger logger =
        LoggerFactory.getLogger( CapacityHelper.class );

    /**
     * NOTE::
     *
     * for beta we are going to send back one data point per day for 30 days.
     * If there are no data points for a given day, send back a
     * datapoint(x=0,y=epoch); where epoch representing the datapoint timestamp.
     */

    /**
     * default constructor
     */
    public CapacityHelper() {
    }

    /**
     * @param groupedByVolume  teh query results grouped by volume
     * @param query the {@link com.formationds.om.repository.query.QueryCriteria
     *
     * @return Returns the @[link List} of {@link Series} representing capacity
     *
     * @throws org.apache.thrift.TException any unhandled thrift error
     */
    public List<Series> processCapacity(
        final Map<String, List<VolumeDatapoint>> groupedByVolume,
        final QueryCriteria query )
        throws TException {



        final List<Series> series = new ArrayList<>();
        final List<Series> logical = new ArrayList<>( );
        final List<Series> physical = new ArrayList<>( );

        series.addAll( logical );
        series.addAll( physical );
        return series;
    }

    protected boolean isPhysicalBytes( final VolumeDatapoint vdp ) {
        return Metrics.PBYTES.key().equalsIgnoreCase( vdp.getKey() );

    }

    protected boolean isLogicalBytes( final VolumeDatapoint vdp ) {
        return Metrics.LBYTES.key().equalsIgnoreCase( vdp.getKey() );

    }
}
