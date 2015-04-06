/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import org.influxdb.InfluxDB;
import org.influxdb.InfluxDBFactory;
import org.influxdb.dto.Serie;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class InfluxDBConnection {
    public static final Logger logger = LoggerFactory.getLogger( InfluxDBConnection.class );

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
        if ( influxDB != null ) {
            influxDB = null;
        }
    }

    /**
     *
     * @return the InfluxDB api connection
     */
    synchronized public InfluxDB connect() {
        if ( influxDB == null ) {
            influxDB = InfluxDBFactory.connect( url, user, String.valueOf( credentials ) );
        }
        return influxDB;
    }

    /**
     *
     * @param timeout the connection timeout
     * @param unit the timeout units.
     *
     * @return the InfluxDB object representing the underlying connection
     *
     * @throws InterruptedException if connection attempt is interrupted
     * @throws TimeoutException if the specified timeout expires before a connection is obtained.
     * @throws IllegalStateException
     */
    public InfluxDB connect(long timeout, TimeUnit unit) throws InterruptedException, TimeoutException {
        CompletableFuture<InfluxDB> f = connectWithRetry();
        try {

            return f.get( timeout, unit );

        } catch (ExecutionException ee ) {

            Throwable throwable = ee.getCause();
            if ( throwable instanceof RuntimeException ) {
                throw (RuntimeException) throwable;
            } else {
                throw new IllegalStateException( "Failed to connect to influx repository.", throwable );
            }

        } finally {

            // cancel the connection attempt if it was not successful (no effect if it completed).
            f.cancel( true );

        }
    }

    /**
     * @return a future that will contain the connection once available
     */
    protected CompletableFuture<InfluxDB> connectWithRetry() {
        synchronized ( this ) {
            if ( influxDB != null ) {
                return CompletableFuture.completedFuture( influxDB );
            }
        }

        Random rng = new Random( System.currentTimeMillis() );
        final int minDelay = 500;
        final int maxDelay = 30000;

        return CompletableFuture.supplyAsync( () -> {
            int cnt = 0;
            InfluxDB conn = null;
            do {
                try {
                    conn = InfluxDBFactory.connect( url, user, String.valueOf( credentials ) );
                } catch ( Exception e ) {
                    try {
                        if ( cnt % 20 == 0 ) {
                            logger.trace( "{} Connection attempts failed.  InfluxDB is not ready.  Retrying",
                                          (cnt == 0 ? "" : cnt) );
                        }
                        Thread.sleep( Math.max( minDelay, rng.nextInt( maxDelay ) ) );
                    } catch ( InterruptedException ie ) {
                        // reset interrupt
                        Thread.currentThread().interrupt();
                        throw new IllegalStateException( "InfluxDB connection retry interrupted." );
                    }
                }
                cnt++;
            }
            while ( conn == null );

            if (conn != null) {
                synchronized ( this ) {
                    influxDB = conn;
                }
            }

            return conn;
        } );
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
