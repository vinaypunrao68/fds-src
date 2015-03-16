/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.commons.crud.AbstractRepository;

import org.influxdb.InfluxDB;
import org.influxdb.InfluxDBFactory;
import org.influxdb.dto.Database;

import java.io.Serializable;
import java.util.Arrays;
import java.util.Properties;

abstract public class InfluxRepository<T,PK extends Serializable> extends AbstractRepository<T, PK> {

//    public static void main( String... args ) {
//        Random rng = new Random( System.currentTimeMillis() );
//        InfluxRepository repository = new InfluxRepository( "http://localhost:8086", "root", "root".toCharArray() );
//
//        InfluxDatabase db = new Builder( "om-metricdb" )
//                                .addShardSpace( "default", "30d", "1d", "/^[a-zA-Z].*/", 1, 1 )
//                                .build();
//        try {
//            // command is silently ignored if the database already exists.
//            if ( !repository.createDatabase( db ) ) {
//                System.out.println( "Database " + db.getName() + " already exists." );
//            }
//            InfluxDB in = repository.adminConnect();
//
//            Metrics[] metrics = Metrics.values();
//            List<String> metricNames = Arrays.stream( Metrics.values() )
//                                             .map( Enum::name )
//                                             .collect( Collectors.toList() );
//            //                  .toArray( new String[metrics.length] );
//
//            List<String> volMetricNames = new ArrayList<>();
//            volMetricNames.add( "domain" );
//            volMetricNames.add( "volume_id" );
//            volMetricNames.add( "volume_name" );
//            volMetricNames.addAll( metricNames );
//
//            Object[] metricValues = new Object[volMetricNames.size()];
//
//            for ( int i = 0; i < 100000; i++ ) {
//                for ( int v = 0; v < 10; v++ ) {
//                    metricValues[0] = "";
//                    metricValues[1] = Integer.toString( v );
//                    metricValues[2] = "volume" + v;
//
//                    Arrays.fill( metricValues, 3, metricValues.length - 1, rng.nextFloat() );
//
//                    Serie serie = new Serie.Builder( "volume_stats" )
//                                      .columns( volMetricNames.toArray( new String[volMetricNames.size()] ) )
//                                      .values( metricValues )
//                                      .build();
//
//                    in.write( "om-influx", TimeUnit.SECONDS, serie );
//                }
//                Thread.sleep( 1000 );
//            }
//        } catch ( Throwable t ) {
//            t.printStackTrace();
//        }
//    }

    public static final String CP_USER = "om.repository.username";
    public static final String CP_CRED = "om.repository.cred";
    public static final String CP_DBNAME = "om.repository.db";

    public static final String TIMESTAMP_COLUMN_NAME = "time";
    
    protected static final String SELECT = "select";
    protected static final String FROM = "from";
    protected static final String WHERE = "where";
    protected static final String AND = "and";
    protected static final String OR = "or";
    
    private final String url;
    private final String adminUser;

    // TODO: store in auth token and use key to decrypt when needed
    // private final AuthenticationToken adminAuth;
    private char[] adminCredentials;

    private Properties connectionProperties;
    private InfluxDBConnection connection;

    /**
     * @param url
     * @param adminUser
     * @param adminCredentials
     */
    protected InfluxRepository( String url, String adminUser, char[] adminCredentials ) {
        this.url = url;
        this.adminUser = adminUser;
        this.adminCredentials = Arrays.copyOf( adminCredentials, adminCredentials.length );
    }

    /**
     * Open the influx database
     * @param properties the connection properties
     */
    @Override
    synchronized public void open( Properties properties ) {
        this.connectionProperties = properties;
        this.connection = new InfluxDBConnection( url,
                                                  properties.getProperty( CP_USER ),
                                                  properties.getProperty( CP_CRED ).toCharArray(),
                                                  properties.getProperty( CP_DBNAME ) );
    }

    /**
     * @return the connection (null if not open)
     */
    public InfluxDBConnection getConnection() {
        return this.connection;
    }

    @Override
    public void close() {
        super.close();
        this.connection = null;
    }

    /**
     * @return an admin connection to the influx repository, using the pre-configured admin user and credentials.
     */
    private InfluxDB adminConnect() {
        return connect( adminUser, adminCredentials );
    }

    /**
     * Connect to InfluxDB
     *
     * @param user
     * @param pwd
     *
     * @return
     */
    private InfluxDB connect( String user, char[] pwd ) {
        return InfluxDBFactory.connect( url, user, String.valueOf( pwd ) );
    }

    /**
     * @param database
     *
     * @return true if the database was successfully created.  False if it already exists.
     *
     * @throws ??? zero documentation on exceptions in influxdb-java api
     */
    protected boolean createDatabase( InfluxDatabase database ) {
        InfluxDB admin = adminConnect();

        // get list of existing database and check for a database with the specified name
        for ( Database db : admin.describeDatabases() ) {
            if ( database.getName().equals( db.getName() ) ) {
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
