/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen.influxdb;

import org.influxdb.InfluxDB;
import org.influxdb.InfluxDBFactory;
import org.influxdb.dto.Serie;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.nurkiewicz.asyncretry.AsyncRetryExecutor;

import java.net.ConnectException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;;

/**
 * Represent a connection to the InfluxDB database.
 * <p>
 * <b>Note:</b>
 * The implementation of the InfluxDBReader and InfluxDBWriter interfaces optionally serialize reads and/or writes
 * against the influxdb database.  The default configuration is to allow concurrent reads and writes, but we have
 * observed problems with InfluxDB handling concurrent operations so this is in place to workaround those issues.
 * <ul>
 * <li>If the serialize_writes flag is set, then we use a write lock in the DBWriter to prevent concurrent writes.</li>
 * <li>Otherwise, we return a no-op implementation of a lock that does no locking at all</li>
 * </ul>
 */
public class InfluxDBConnection {
    public static final Logger logger = LoggerFactory.getLogger( InfluxDBConnection.class );

    public static final String FDS_OM_INFLUXDB_ENABLE_BACKTRACE = "fds.om.influxdb.enable_query_backtrace";
    public static final String FDS_OM_INFLUXDB_SERIALIZE_READS  = "fds.om.influxdb.serialize_reads";
    public static final String FDS_OM_INFLUXDB_SERIALIZE_WRITES = "fds.om.influxdb.serialize_writes";
    public static final String FDS_OM_INFLUXDB_SERIALIZE_ALL    = "fds.om.influxdb.serialize_all";

    /**
     * @return the value of the fds.om.influxdb.serialize_writes flag or false if it is not defined
     */
    private static boolean isSerializeWrites()
    {
        return false;
    }

    private final LoggingInfluxDBReader reader = new LoggingInfluxDBReader();
    private final LoggingInfluxDBWriter writer = new LoggingInfluxDBWriter();

    // NOOP lock that grants a "lock" no matter what.  This is designed to look like a Lock
    // for use in code that is configurable as to whether it locks or not and avoids the need
    // for a separate if/else blocks with otherwise identical code.
    private final Lock noLock = new Lock()
    {
        // @formatter:off
        @Override public void lock() {}
        @Override public void lockInterruptibly() throws InterruptedException {}
        @Override public boolean tryLock() { return true; }
        @Override public boolean tryLock( long time, TimeUnit unit ) throws InterruptedException { return true; }
        @Override public void unlock() {}
        /** string with lock id and [NOOP] */
        public String toString() { return "NOOP[" + System.identityHashCode( this ) + "]"; }
        // @formatter:on
        @Override public Condition newCondition()
        {
            throw new UnsupportedOperationException( "condition not supported" );
        }
    };

    // @formatter:off
    // overrides toString to return the lock name and identity hash code only. used for trace logging only
    private class NamedLock extends ReentrantLock {
        private final String name;
        public NamedLock(String n) { super(); this.name = n; }
        public String toString() { return name + "[" + System.identityHashCode( this ) + "]"; }
    }
    // @formatter:on

    private final ReentrantLock writeLock  = new NamedLock( "WRITE" );

    private final String url;
    private final String user;
    // TODO: convert to authtoken or otherwise encrypt
    private final char[] credentials;

    // optional.  If not provided than this is an admin connection
    private final Optional<String> database;

    private CompletableFuture<InfluxDB> influxDB = null;

    /**
     *
     * @param url the influxdb url
     * @param user the influx user
     * @param credentials the influx user credentials
     */
    InfluxDBConnection( String url, String user, char[] credentials ) {
        this(url, user, credentials, null);
    }

    /**
     * @param url the influxdb url
     * @param user the influx user
     * @param credentials the influx user credentials
     * @param database the influx database
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

    protected CompletableFuture<InfluxDB> connectWithRetry() {

        final AsyncRetryExecutor asyncRetryExecutor =
                new AsyncRetryExecutor(
                    Executors.newSingleThreadScheduledExecutor( ) );

            asyncRetryExecutor.withExponentialBackoff( 500, 2 )
                              .withMaxDelay( 10_000 )
                              .withUniformJitter( )
                              .retryInfinitely()
                              .retryOn( ExecutionException.class )
                              .retryOn( RuntimeException.class )
                              .retryOn( ConnectException.class );

            return asyncRetryExecutor.getWithRetry( ( ) -> {
                                           InfluxDB conn
                                               = InfluxDBFactory.connect( url,
                                                                          user,
                                                                          String.valueOf( credentials ) );

                                           // attempt to ping the server to make sure it is really there.
                                           conn.ping( );

                                           return conn;
                                       } );
    }

    /**
     * @return the influxdb reader
     */
    public InfluxDBReader getDBReader()
    {
        if ( !database.isPresent() )
        {
            throw new IllegalStateException( "Can't create reader for system database." );
        }
        return reader;
    }

    /**
     *
     * @return the influxdb writer
     */
    public InfluxDBWriter getDBWriter() {
        if ( !database.isPresent() ) {
            throw new IllegalStateException( "Can't create writer for system database." );
        }

        return writer::write;
    }

    public InfluxDBWriter getAsyncDBWriter()
    {
        if ( !database.isPresent() )
        {
            throw new IllegalStateException( "Can't create writer for system database." );
        }
        return writer::writeAsync;
    }

    /**
     * @return the lock to use for queries.
     *
     * @see InfluxDBConnection for a description of the locking strategy
     */
    private Lock getWriteLock()
    {
        if ( isSerializeWrites() )
        {
            return writeLock;
        }
        else
        {
            return noLock;
        }
    }

    /**
     * timedOp( () -> { doSomething(); } );
     *
     * @return number of milliseconds
     */
    // todo: something similar for a result-bearing operation...
    private static long timedOp( Runnable r )
    {
        long start = System.currentTimeMillis();
        r.run();
        return System.currentTimeMillis() - start;
    }

    /**
     * Implementation of the DB Writer interface that logs write operations at a trace level and also tracks write time
     * stats.  Actual write operations are guarded by an optional lock as described in {@link InfluxDBConnection}
     */
    private class LoggingInfluxDBWriter implements InfluxDBWriter
    {
        public final Logger writeLogger = LoggerFactory.getLogger( LoggingInfluxDBWriter.class );

        @Override public void write( TimeUnit precision, Serie... series )
        {
            doWrite( CompletableFuture.supplyAsync( InfluxDBConnection.this::connect ), precision, series );
        }

        public void writeAsync( TimeUnit precision, Serie... series )
        {
            doWrite( getAsyncConnection(), precision, series );
        }

        private void doWrite( CompletableFuture<InfluxDB> connSupplier, TimeUnit precision, Serie... series )
        {
            long start = System.currentTimeMillis();
            long connTime = -1;
            long writeTime = -1;
            long lockTime = -1;
            Throwable failed = null;

            Lock lock = getWriteLock();
            writeLogger.trace( "WRITE_BEGIN [{}]: {} series", lock, series.length );

            try
            {
                if ( Boolean.valueOf( System.getProperty( FDS_OM_INFLUXDB_ENABLE_BACKTRACE, "false" ) ) )
                {
                    // hack to log the stack trace of each query
                    StackTraceElement[] st = new Exception().getStackTrace();
                    int maxDepth = 25;
                    int d = 0;
                    for ( StackTraceElement e : st )
                    {
                        writeLogger.trace( "WRITE_TRACE[" + d + "]: {}", e );
                        if ( ++d > maxDepth || e.getClassName().startsWith( "org.eclipse.jetty" ) ) { break; }
                    }
                }

                InfluxDB conn = connSupplier.get();
                connTime = System.currentTimeMillis() - start;

                lockTime = timedOp( lock::lock );
                try
                {
                    writeTime = timedOp( () -> conn.write( database.get(), precision, series ) );
                }
                finally
                {
                    lock.unlock();
                }
            }
            catch ( Throwable e )
            {
                failed = e;
                writeLogger.trace( "WRITE_FAIL  [{}]: {} [ex={}; conn={} ms; lock={} ms; write={} ms]", start,
                                   series.length, e.getMessage(), connTime, lockTime, writeTime );

                if ( e instanceof RuntimeException ) { throw (RuntimeException) e; }
                else { throw new RuntimeException( e ); }
            }
            finally
            {
                writeLogger.trace( "WRITE_END   [{}]:[{}]: {} [result={}; conn={} ms; lock={} ms, write={} ms]", lock,
                                   start, series.length, ( failed != null ? "'" + failed.getMessage() + "'" : "" ),
                                   lockTime, connTime, writeTime );
            }
        }
    }

    /**
     * Implementation of the DB Reader that logs trace messages for Query execution
     * when trace logging is enabled.  Read operations are guarded by an optional lock
     * as described in {@link InfluxDBConnection}
     */
    private class LoggingInfluxDBReader implements InfluxDBReader
    {
        public final Logger queryLogger = LoggerFactory.getLogger( LoggingInfluxDBReader.class );

        LoggingInfluxDBReader() { }

        public List<Serie> query( String query, TimeUnit precision )
        {
            long start = System.currentTimeMillis();
            long connTime = -1;
            long queryTime = -1;
            long lockTime = -1;
            List<Serie> result = Collections.emptyList();
            Throwable failed = null;
            try
            {
                queryLogger.trace( "QUERY_BEGIN [{}]: {}", start, query );

                if ( Boolean.valueOf( System.getProperty( FDS_OM_INFLUXDB_ENABLE_BACKTRACE, "false" ) ) )
                {
                    // hack to log the stack trace of each query
                    StackTraceElement[] st = new Exception().getStackTrace();
                    int maxDepth = 25;
                    int d = 0;
                    for ( StackTraceElement e : st )
                    {
                        queryLogger.trace( "QUERY_TRACE[" + d + "]: {}", e );
                        if ( ++d > maxDepth || e.getClassName().startsWith( "org.eclipse.jetty" ) ) { break; }
                    }
                }

                InfluxDB conn = connect();
                connTime = System.currentTimeMillis() - start;
                try
                {
                    result = conn.query( database.get(), query, precision );
                }
                finally
                {
                }
                queryTime = System.currentTimeMillis() - start - connTime;

                return result;
            }
            catch ( Throwable e )
            {
                // This is expected if we haven't yet written any data to the database
                // for the series in the query.
                String msg = e.getMessage();
                if ( msg != null && msg.startsWith( "Couldn't find series:" ) )
                {
                    // empty list
                    return result;
                }
                else
                {
                    failed = e;
                    queryLogger
                            .trace( "QUERY_FAIL  [{}]: {} [ex={}; conn={} ms; query={} ms]", start, query,
                                    e.getMessage(), connTime, queryTime );
                    throw e;
                }
            }
            finally
            {
                queryLogger.trace( "QUERY_END   [{}]: {} [result={}; conn={} ms; query={} ms]",
                                   start, query, ( failed != null
                                                   ? "'" + failed.getMessage() + "'"
                                                   : Integer.toString( result.size() ) ), connTime,
                                   queryTime );
            }
        }
    }
}
