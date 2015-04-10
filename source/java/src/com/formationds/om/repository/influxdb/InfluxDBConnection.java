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
import java.util.Collections;
import java.util.List;
import java.util.Optional;
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

    // optional.  If not provided than this is an admin connection
    private final Optional<String> database;

    private CompletableFuture<InfluxDB> influxDB = null;

    /**
     *
     * @param url
     * @param user
     * @param credentials
     */
    InfluxDBConnection( String url, String user, char[] credentials ) {
        this(url, user, credentials, null);
    }

    /**
     * @param url
     * @param user
     * @param credentials
     * @param database
     */
    InfluxDBConnection( String url, String user, char[] credentials, String database ) {
        this.url = url;
        this.user = user;
        this.credentials = Arrays.copyOf( credentials, credentials.length );
        this.database = ((database != null && !database.trim().isEmpty() ) ?
                         Optional.of( database ) :
                         Optional.empty() );
    }

    public String getUrl() { return url; }
    public String getUser() { return user; }
    char[] getCredentials() { return Arrays.copyOf( credentials, credentials.length ); }

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
     *
     * @throws RuntimeException if the connection is not immediately available
     */
    public InfluxDB connect() {
        try {
            return connect( 500, TimeUnit.MILLISECONDS );
        } catch (TimeoutException ee) {
            throw new IllegalStateException( "Connection to influx repository is not available." );
        } catch (InterruptedException ie) {
            // reset interrupt status
            Thread.currentThread().interrupt();
            throw new IllegalStateException( "Connection to influx repository interrupted." );
        }
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
     * @throws IllegalStateException if an error occurs connecting
     */
    public InfluxDB connect(long timeout, TimeUnit unit) throws InterruptedException, TimeoutException {

        CompletableFuture<InfluxDB> f = getAsyncConnection();
        try {

            // if the timeout is zero, use getNow().  Otherwise wait for up to the specified amount of time.
            // (this is required because CompletableFuture will throw a TimeoutException if the timeout reaches 0, even
            // if it is completed.  An alternative is to convert the timeout to something minuscule like 1 nano)
            if ( timeout <= 0 ) {
                InfluxDB db = f.getNow( null );
                if ( db == null ) {
                    throw new IllegalStateException( "Connection to influx repository is not available." );
                }
                return db;
            }
            return f.get( timeout, unit );

        } catch (ExecutionException ee ) {

            influxDB = null;
            Throwable throwable = ee.getCause();
            if ( throwable instanceof RuntimeException ) {
                throw (RuntimeException) throwable;
            } else {
                throw new IllegalStateException( "Failed to connect to influx repository.", throwable );
            }

        }
    }

    synchronized protected CompletableFuture<InfluxDB> getAsyncConnection() {
        if (influxDB == null) {
            influxDB = connectWithRetry();
        }
        return influxDB;
    }

    /**
     * @return a future that will contain the connection once available
     */
    protected CompletableFuture<InfluxDB> connectWithRetry() {

        Random rng = new Random( System.currentTimeMillis() );
        final int minDelay = 500;
        final int maxDelay = 10000;

        return CompletableFuture.supplyAsync( () -> {
            int cnt = 0;
            InfluxDB conn = null;
            do {
                try {

                    conn = InfluxDBFactory.connect( url, user, String.valueOf( credentials ) );

                    // attempt to ping the server to make sure it is really there.
                    conn.ping();

                } catch ( Exception e ) {

                    conn = null;
                    try {

                        final int delay = Math.max( minDelay, rng.nextInt( maxDelay ) );
                        if ( cnt % 20 == 0 ) {
                            logger.trace( "{} Connection attempts failed.  InfluxDB is not ready.  Waiting {} ms before retrying",
                                          (cnt == 0 ? "" : cnt),
                                          delay );
                        }
                        Thread.sleep( delay );

                    } catch ( InterruptedException ie ) {

                        // reset interrupt
                        Thread.currentThread().interrupt();
                        throw new IllegalStateException( "InfluxDB connection retry interrupted." );

                    }
                }
                cnt++;
            }
            while ( conn == null );

            return conn;
        } );
    }

    public InfluxDBWriter getDBWriter() {
        if ( !database.isPresent() ) {
            throw new IllegalStateException( "Can't create writer for system database." );
        }
        return new InfluxDBWriter() {
            @Override
            public void write( TimeUnit precision, Serie... series ) {
                connect().write( database.get(), precision, series );
            }
        };
    }

    public InfluxDBReader getDBReader()
    {
        if ( !database.isPresent() ) {
            throw new IllegalStateException( "Can't create reader for system database." );
        }
        return new InfluxDBReader() {
            @Override
            public List<Serie> query( String query, TimeUnit precision )
            {
                try {

                    return connect().query( database.get(), query, precision );

                } catch ( RuntimeException e ) {

                    // This is expected if we haven't yet written any data to the database
                    // for the series in the query.
                    String msg = e.getMessage();
                    if ( msg != null && msg.startsWith("Couldn't find series:") ) {

                        return Collections.emptyList();

                    } else {

                        throw e;

                    }
                } catch ( Throwable t ) {
                    throw t;
                }

            }
        };
    }

    public InfluxDBWriter getAsyncDBWriter() {
        if ( !database.isPresent() ) {
            throw new IllegalStateException( "Can't create writer for system database." );
        }
        return new InfluxDBWriter() {
            @Override
            public void write( TimeUnit precision, Serie... series ) {
                getAsyncConnection().thenAccept( ( conn ) -> {
                    conn.write( database.get(), precision, series );
                } );
            }
        };
    }
}
