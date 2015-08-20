package com.formationds.om.webkit.rest.v08.metrics;

import java.io.IOException;
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
	
	private StatsStream(){
		
		try {
			StatsConnection conn = StatsConnection.newConnection( "localhost", 11011, "guest", "guest" );
			
			subscription = conn.subscribe( "1.0.stats.volume", this );
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public static StatsStream getInstance(){
		
		if ( instance == null ){
			instance = new StatsStream();
		}
		
		return instance;
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
	public void messageReceived(StatDataPoint datapoint) {
		
		logger.info( "Stat message received: " + datapoint.toJson() );
		getStatCache().put( datapoint.getReportTime(), datapoint );
	}
	
	private Subscription getSubscription(){
		return subscription;
	}
}
