/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.model.Series;
import com.formationds.commons.model.entity.VolumeDatapoint;
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
public class PerformanceHelper {

    private static final Logger logger =
        LoggerFactory.getLogger( PerformanceHelper.class );

    /**
     * default constructor
     */
    public PerformanceHelper() {
    }

    /**
     * @param query the {@link com.formationds.om.repository.query.QueryCriteria
     *
     * @return Returns the @[link List} of {@link Series} representing performance
     *
     * @throws org.apache.thrift.TException any unhandled thrift error
     */
    public List<Series> processPerformance(
        final Map<String, List<VolumeDatapoint>> groupedByVolume,
        final QueryCriteria query )
        throws TException {

        final List<Series> series = new ArrayList<>();

        groupedByVolume.forEach( ( volume, datapoints ) -> {


        } );


        return series;
    }
}
