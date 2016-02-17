/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.client.v08.model.stats.Datapoint;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * @author ptinius
 */
public class DatapointHelper {

    private static final Logger logger =
        LoggerFactory.getLogger( DatapointHelper.class );

    /**
     * default constructor
     */
    public DatapointHelper() {
    }

    /**
     * @param datapoints the {@link java.util.List} of {@link com.formationds.commons.model.Datapoint}
     */
    public void sortByY( final List<Datapoint> datapoints ) {
        Collections.sort( datapoints, Comparator.comparing( Datapoint::getY ) );
    }

    /**
     * @param datapoints the {@link List} of {@link com.formationds.commons.model.Datapoint}
     */
    public void sortByX( final List<Datapoint> datapoints ) {
        Collections.sort( datapoints, Comparator.comparing( Datapoint::getX ) );
    }

    /**
     * @param start the {@code long} representing the starting date range, unix epoch
     * @param interval the {@link long} representing the interval at which the
     *                 datapoints are created, {@link java.util.concurrent.TimeUnit#SECONDS}
     * @param count the {@code count} representing the number of datapoints
     * @param datapoints the {@link List} of {@link com.formationds.commons.model.Datapoint}
     */
    public void padWithEmptyDatapoints( final long start,
                                        final long interval,
                                        final int count,
                                        final List<Datapoint> datapoints ) {

        long epoch = start;
        for( int i = 0; i < count; i++ ) {

            Datapoint dp = new Datapoint();
            dp.setX(  new Double( epoch ) );
            dp.setY(  0.0 );
            
            datapoints.add( i, dp );
            
            epoch += interval;
        }
    }
}
