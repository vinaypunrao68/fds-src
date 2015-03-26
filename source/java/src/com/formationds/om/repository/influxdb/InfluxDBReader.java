/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import org.influxdb.dto.Serie;

import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 *
 */
public interface InfluxDBReader
{
    /**
     * Execute the specified query and return an ordered list of the Series
     * meeting the query criteria
     *
     * @param query the query to execute
     * @param precision the precision used for the values.
     *
     * @return the list of series that match the query
     */
    public List<Serie> query( String query, TimeUnit precision );
}
