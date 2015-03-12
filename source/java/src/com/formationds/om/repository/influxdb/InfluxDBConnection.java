/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import org.influxdb.InfluxDB;
import org.influxdb.InfluxDBFactory;
import org.influxdb.dto.Serie;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;

class InfluxDBConnection
{
    private final String url;
    private final String user;
    // TODO: convert to authtoken or otherwise encrypt
    private final char[] credentials;
    private final String database;

    private InfluxDB influxDB;

    InfluxDBConnection( String url, String user, char[] credentials, String database ) {
        this.url = url;
        this.user = user;
        this.credentials = Arrays.copyOf( credentials, credentials.length );
        this.database = database;
    }

    /**
     * close the influxdb connection.
     * <p/>
     * Influx-java does not support any close/disconnect api so this does nothing more than
     * clear the reference to the InfluxDB object.
     */
    synchronized public void close() {
        if (influxDB != null) {
            influxDB = null;
        }
    }

    synchronized protected InfluxDB connect() {
        if ( influxDB == null ) {
            influxDB = InfluxDBFactory.connect( url, user, String.valueOf( credentials ) );
        }
        return influxDB;
    }

    public InfluxDBWriter getDBWriter() {
        return new InfluxDBWriter() {
            @Override
            public void write( TimeUnit precision, Serie... series ) {
                connect().write( database, precision, series );
            }
        };
    }

    public InfluxDBReader getDBReader()
    {
        return new InfluxDBReader() {
            @Override
            public List<Serie> query( String query, TimeUnit precision )
            {
                return connect().query( database, query, precision );
            }
        };
    }
}
