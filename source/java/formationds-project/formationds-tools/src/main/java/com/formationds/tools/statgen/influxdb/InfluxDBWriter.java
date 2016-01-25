/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen.influxdb;

import org.influxdb.dto.Serie;

import java.util.Collection;
import java.util.concurrent.TimeUnit;

/**
 *
 */
public interface InfluxDBWriter
{
    /**
     * @param precision the precision used for values
     * @param series the list of series to write
     */
    public void write( final TimeUnit precision, final Serie... series );

    /**
     * @param precision the precision used for values
     * @param series the list of series to write
     */
    default public void write( final TimeUnit precision, final Collection<Serie> series )
    {
        write( precision, series.toArray( new Serie[series.size()] ) );
    }
}
