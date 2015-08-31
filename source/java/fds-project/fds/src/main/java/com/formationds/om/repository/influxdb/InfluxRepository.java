/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.om.repository.influxdb;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.crud.AbstractRepository;
import com.formationds.commons.model.DateRange;
import com.formationds.om.repository.query.QueryCriteria;
import org.influxdb.InfluxDB;
import org.influxdb.dto.Database;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Serializable;
import java.util.Iterator;
import java.util.Properties;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;

abstract public class InfluxRepository<T,PK extends Serializable> extends AbstractRepository<T, PK> {

    public static final Logger logger = LoggerFactory.getLogger( InfluxRepository.class );

    public static final String CP_USER   = "om.repository.username";
    public static final String CP_CRED   = "om.repository.cred";
    public static final String CP_DBNAME = "om.repository.db";

    public static final String TIMESTAMP_COLUMN_NAME = "time";
    public static final String SEQUENCE_COLUMN_NAME = "sequence_number";

    protected static final String SELECT = "select ";
    protected static final String FROM   = " from ";
    protected static final String WHERE  = " where ";
    protected static final String AND    = " and ";
    protected static final String OR     = " or ";

    private InfluxDBConnection adminConnection;

    private Properties         dbConnectionProperties;
    private InfluxDBConnection dbConnection;

    /**
     * @param url the url
     * @param adminUser the admin user
     * @param adminCredentials the credentials
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

    /**
     * Convert the Java TimeUnit to an InfluxDB unit.
     * <p/>
     * InfluxDB's week (w) specifier is not supported in java.util.Concurrent.TimeUnit
     * and InfluxDB does not support Nanosecond precision.
     *
     * @param unit the java.util.Concurrent TimeUnit
     * @return the influx unit specifier
     *
     * @throws IllegalArgumentException for TimeUnit.NANOSECONDS which is not supported by influxdb.
     */
    public String toInfluxUnits(TimeUnit unit) {
        switch ( unit ) {
            case MICROSECONDS: return "u";
            case MILLISECONDS: return "ms";
            case SECONDS:      return "s";
            case MINUTES:      return "m";
            case HOURS:        return "h";
            case DAYS:         return "d";

            case NANOSECONDS:
            default:
                // Influx supports time to microseconds
                // and also supports a "weeks" options (w), but
                // juc TimeUnit does not support that.
                throw new IllegalArgumentException( "Unsupported InfluxDB TimeUnit: " + unit );
        }
    }

    /**
     * Method to create a string from the query object that matches influx format
     *
     * @param queryCriteria the query criteria
     * @param volIdColumnName the name for the volume id column, required if the query criteria has volume contexts.
     *
     * @return a query string for influx event series based on the criteria
     */
    protected String formulateQueryString( QueryCriteria queryCriteria, String volIdColumnName ) {

        StringBuilder sb = new StringBuilder();

        // * if no columns, otherwise comma-separated list of column names
        String projection = queryCriteria.getColumnString();

        String prefix = SELECT + projection + FROM + getEntityName();
        sb.append( prefix );

        if ( queryCriteria.getRange() != null ||
             queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {

            sb.append( WHERE );
        }

        // do time range
        if ( queryCriteria.getRange() != null ) {

            DateRange range = queryCriteria.getRange();
            if ( range.getStart() == null )
                throw new IllegalArgumentException( "DateRange must have a start time" );

            if ( range.getEnd() != null )
                sb.append( "( " );

            String influxUnit = toInfluxUnits( range.getUnit() );
            sb.append( getTimestampColumnName() ).append( " > " )
              .append( range.getStart() )
              .append( influxUnit );

            if ( range.getEnd() != null ) {
                sb.append( AND )
                  .append( getTimestampColumnName() )
                  .append( " < " )
                  .append( range.getEnd() ).append( influxUnit )
                  .append( " )" );
            }
        }

        if ( queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {

            if ( queryCriteria.getRange() != null )
                sb.append( AND );

            sb.append( "( " );
            Iterator<Volume> contextIt = queryCriteria.getContexts().iterator();
            while ( contextIt.hasNext() ) {

                Volume volume = contextIt.next();

                sb.append( volIdColumnName ).append( " = " ).append( volume.getId() );

                if ( contextIt.hasNext() ) {
                    sb.append( OR );
                }
            }

            sb.append( " )" );
        }

        return sb.toString();
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
     * @param r the function to run once the connection is available
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
     * @throws RuntimeException ??? zero documentation on exceptions in influxdb-java api
     */
    protected boolean createDatabase( InfluxDatabase database ) {
        InfluxDB admin = adminConnect();
        return createDatabase( admin, database );
    }

    /**
     *
     * @param admin the admin connection
     * @param database the database
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

    /**
     *
     * @return the timestamp column name
     */
    public String getTimestampColumnName()
    {
        return TIMESTAMP_COLUMN_NAME;
    }

    /**
     *
     * @return the sequence column name
     */
    public String getSequenceColumnName()
    {
        return SEQUENCE_COLUMN_NAME;
    }

    /**
     *
     * @param col the column name
     * @return true if the column name matches the timestamp column name
     */
    public boolean isTimestampColumn(String col)
    {
        return getTimestampColumnName().equalsIgnoreCase( col );
    }

    /**
     *
     * @param col the column name
     *
     * @return true if the column name matches the sequence column name
     */
    public boolean isSequenceColumn(String col) {
        return getSequenceColumnName().equalsIgnoreCase( col );
    }
}
