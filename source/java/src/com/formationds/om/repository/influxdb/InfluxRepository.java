/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.commons.crud.AbstractRepository;

import org.influxdb.InfluxDB;
import org.influxdb.dto.Database;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Serializable;
import java.util.Properties;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.ExecutionException;
import java.util.function.Function;

abstract public class InfluxRepository<T,PK extends Serializable> extends AbstractRepository<T, PK> {

    public static final Logger logger = LoggerFactory.getLogger( InfluxRepository.class );

    public static final String CP_USER   = "om.repository.username";
    public static final String CP_CRED   = "om.repository.cred";
    public static final String CP_DBNAME = "om.repository.db";

    public static final String TIMESTAMP_COLUMN_NAME = "time";
    public static final String SEQUENCE_COLUMN_NAME = "sequence";

    protected static final String SELECT = "select";
    protected static final String FROM   = "from";
    protected static final String WHERE  = "where";
    protected static final String AND    = "and";
    protected static final String OR     = "or";

    private InfluxDBConnection adminConnection;

    private Properties         dbConnectionProperties;
    private InfluxDBConnection dbConnection;

    /**
     * @param url
     * @param adminUser
     * @param adminCredentials
     */
    protected InfluxRepository( String url, String adminUser, char[] adminCredentials ) {
        this.adminConnection =new InfluxDBConnection( url, adminUser, adminCredentials );
    }

    abstract public String getInfluxDatabaseName();

    private Properties getDefaultConnectionProperties() {
        Properties props = new Properties();
        props.setProperty( InfluxRepository.CP_USER, adminConnection.getUser() );
        props.setProperty( InfluxRepository.CP_CRED, String.valueOf( adminConnection.getCredentials() ) );
        return props;
    }

    /**
     * Open the influx database
     * @param properties the dbConnection properties
     */
    @Override
    synchronized public void open( Properties properties ) {

        this.dbConnectionProperties = (properties != null ? properties : getDefaultConnectionProperties());
        this.dbConnection = new InfluxDBConnection( adminConnection.getUrl(),
                                                    dbConnectionProperties.getProperty( CP_USER ),
                                                    dbConnectionProperties.getProperty( CP_CRED ).toCharArray(),
                                                    getInfluxDatabaseName() );
    }

    /**
     * @return the dbConnection (null if not open)
     */
    public InfluxDBConnection getConnection() {
        return this.dbConnection;
    }

    @Override
    public void close() {
        super.close();
        this.dbConnection = null;
    }

    /**
     * @return an admin dbConnection to the influx repository, using the pre-configured admin user and credentials.
     */
    private InfluxDB adminConnect() {
        try {
            return adminConnection.getAsyncConnection().get();
        } catch ( InterruptedException e ) {
            Thread.currentThread().interrupt();
            return null;
        } catch ( ExecutionException e ) {
            Throwable t = e.getCause();
            if (t instanceof RuntimeException) throw (RuntimeException)t;
            else throw new IllegalStateException( "Failed to connect to InfluxDB", t );
        }
    }

    /**
     * Run the specified operation once a connection is available.
     *
     * @param r
     *
     * @return a completion stage that will contain the result of the function when complete
     */
    private <R> CompletionStage<R> whenConnected(Function<InfluxDB, R> r) {
        return adminConnection.getAsyncConnection().thenApply( r );
    }

    /**
     *
     * @param database the database to create if it does not exist
     *
     * @return a Future that will contain the result of the create database operation.
     */
    protected CompletableFuture<Boolean> createDatabaseAsync( InfluxDatabase database ) {

        return whenConnected( ( admin ) -> createDatabase( admin, database ) ).toCompletableFuture();

    }

    /**
     * @param database the database to create if it does not already exist
     *
     * @return true if the database was successfully created.  False if it already exists.
     *
     * @throws ??? zero documentation on exceptions in influxdb-java api
     */
    protected boolean createDatabase( InfluxDatabase database ) {
        InfluxDB admin = adminConnect();
        return createDatabase( admin, database );
    }

    /**
     *
     * @param admin the admin connection
     * @param database
     * @return true if the atabase was successfully created. False if it already exists
     */
    private boolean createDatabase( InfluxDB admin, InfluxDatabase database )
    {
        // get list of existing database and check for a database with the specified name
        for ( Database db : admin.describeDatabases() ) {
            if ( database.getName().equals( db.getName() ) ) {
                logger.debug( "Database " + database.getName() + " already exists." );
                return false;
            }
        }

        // database does not already exist so create it
        admin.createDatabase( database.getDatabaseConfiguration() );
        return true;
    }

    public String getTimestampColumnName() {
        return TIMESTAMP_COLUMN_NAME;
    }
}
