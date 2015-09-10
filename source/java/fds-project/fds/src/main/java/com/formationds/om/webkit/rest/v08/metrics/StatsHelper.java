package com.formationds.om.webkit.rest.v08.metrics;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.stats_client.StatsConnection;

public class StatsHelper {

    private static final Logger logger =
            LoggerFactory.getLogger( StatsHelper.class );
	private static StatsHelper instance;
	 
	private StatsConnection conn;
	
	private StatsHelper() throws Exception{
		
		connect();
	}
	
	private void connect() throws Exception {
		
		try {
			conn = StatsConnection.newConnection( "localhost", 11011, "admin", "admin" );
			
			logger.info( "StatsStream connected successfully." );
		} catch ( Exception e ){
			
			logger.info( "Could not establish connectivity to the stats service." );
			
			if ( conn != null ){
				conn.close();
			}
			
			instance = null;
			throw e;
		}
	}
	
	public static StatsHelper getInstance() throws Exception{
		
		if ( instance == null ){
			instance = new StatsHelper();
		}
		
		return instance;
	}
	
	public StatsConnection getConnection(){
		return conn;
	}
}
