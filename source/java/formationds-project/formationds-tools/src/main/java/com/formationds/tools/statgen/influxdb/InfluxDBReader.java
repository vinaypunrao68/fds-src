/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen.influxdb;

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
     * <p/>
     * From InfluxDB Docs on time_precision (https://influxdb.com/docs/v0.8/api/reading_and_writing_data.html)
     * <pre>
     *     Time Precision on Returned Results
     *     The time precision of the epoch returned in the time column
     *     can be specified via the time_precision query parameter. By
     *     default time precision is in milliseconds, this is true for
     *     both reading and writing timestamps. To override the time
     *     precision you can be set to either s for seconds, ms for
     *     milliseconds, or u for microseconds.
     * </pre>
     * Note that the time precision unit here is not the same as any units specified in the
     * query itself.
     *
     * @param query the query to execute
     * @param precision the precision used for the time values returned.
     *
     * @return the list of series that match the query
     */
    public List<Serie> query( String query, TimeUnit precision );
}
