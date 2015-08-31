package com.formationds.om.webkit.rest.v08.metrics;

import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.stats_client.StatsConnection;
import com.formationds.stats_client.Subscription;
import com.formationds.stats_client.SubscriptionHandler;
import com.formationds.stats_client.model.StatDataPoint;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;

public class StatsStream implements SubscriptionHandler {

    private static final Logger logger =
            LoggerFactory.getLogger( StatsStream.class );
	private static StatsStream instance;
	
	private LoadingCache<Long, StatDataPoint> statCache;
	private Subscription subscription; 
	private StatsConnection conn;
	
	private StatsStream() throws Exception{
		
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
	
	public void startListening(){
		
		try {
			subscription = conn.subscribeToContextType( "admin", StatDataPoint.CONTEXT_TYPE.VOLUME, this );
		}
		catch( Exception e ){
			logger.warn( "Subscription to admin topic failed.  Publishing should work without problems." );
		}
	}
	
	public static StatsStream getInstance() throws Exception{
		
		if ( instance == null ){
			instance = new StatsStream();
		}
		
		if ( instance.getSubscription() == null ){
			instance.startListening();
		}
		
		return instance;
	}
	
	public StatsConnection getConnection(){
		return conn;
	}
	
	public LoadingCache<Long, StatDataPoint> getStatCache(){
		
		if ( statCache == null ){
			statCache = CacheBuilder.newBuilder()
					.maximumSize( 5000 )
					.expireAfterWrite( 5, TimeUnit.MINUTES )
					.build( new CacheLoader<Long, StatDataPoint>(){

						@Override
						public StatDataPoint load( Long  key ) throws Exception {
							// TODO Auto-generated method stub
							return null;
						}
						
					});
		}
		
		return statCache;
	}

	@Override
	public void messageReceived( StatDataPoint datapoint ) {
		
		logger.info( "Stat message received: " + datapoint.toJson() );
		getStatCache().put( datapoint.getReportTime(), datapoint );
	}
	
	private Subscription getSubscription(){
		return subscription;
	}
}
