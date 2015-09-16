package com.formationds.rest.v08.model.stats;

import java.util.Calendar;

import org.junit.Assert;
import org.junit.Test;

import com.formationds.client.v08.model.stats.ContextDef;
import com.formationds.client.v08.model.stats.ContextType;
import com.formationds.client.v08.model.stats.StatDataPoint;
import com.formationds.client.v08.model.stats.StatTypes;
import com.formationds.client.v08.model.stats.StatDataPoint.TIME_UNITS;

public class TestSerialization {

	@Test
	public void testToJson(){
		
		StatDataPoint datapoint = new StatDataPoint();
		datapoint.setCollectionPeriod( 10L );
		datapoint.setCollectionTimeUnit( TIME_UNITS.HOURS );
		datapoint.setContextType( ContextType.VOLUME );
		datapoint.setContextId( 12345L );
		datapoint.setNumberOfSamples( 6 );
		datapoint.setMaximumValue( 100.0 );
		datapoint.setMinimumValue( 12.7 );
		datapoint.setMetricName( StatTypes.PUTS.name() );
		datapoint.setMetricValue( 76.0 );
		datapoint.setReportTime( Calendar.getInstance().getTimeInMillis() );
		
		String json = datapoint.toJson();
		System.out.println( json );
		
		Assert.assertTrue( json.equals( "{\"minimumValue\":12.7,\"collectionTimeUnit\":\"HOURS\"," + 
				"\"metricName\":\"PUTS\",\"collectionPeriod\":10,\"relatedContexts\":[],\"contextType\":"+ 
				"\"VOLUME\",\"metricValue\":76,\"contextId\":12345,\"numberOfSamples\":6,\"maximumValue\":100,"
				+ "\"reportTime\":" + datapoint.getReportTime() +"}" ) );
		
		datapoint.getRelatedContexts().add( new ContextDef( ContextType.NODE, 34243L ) );
		
		json = datapoint.toJson();
		System.out.println( json );
		
		Assert.assertTrue( json.equals( "{\"minimumValue\":12.7,\"collectionTimeUnit\":\"HOURS\","
				+ "\"metricName\":\"PUTS\",\"collectionPeriod\":10,\"relatedContexts\":[{\"contextType\":\"NODE\",\"contextId\":34243}],"
				+ "\"contextType\":"
				+ "\"VOLUME\",\"metricValue\":76,\"contextId\":12345,\"numberOfSamples\":6,\"maximumValue\":100,"
				+ "\"reportTime\":" + datapoint.getReportTime() +"}" ) );
	}
	
}
