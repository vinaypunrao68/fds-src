package com.formationds.om.repository.influxdb;

import static org.junit.Assert.assertEquals;

import java.util.ArrayList;
import java.util.List;

import org.junit.Test;

import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.om.repository.query.QueryCriteria;

public class TestInfluxRepository {

	@Test
	public void verifyUIQuerySyntax(){
		
		char[] creds = "password".toCharArray();
		InfluxMetricRepository influxRepo = new InfluxMetricRepository( null, null, creds );
		
		System.out.println( "Testing a complicated query very much like what we receive from the UI." );
		
		QueryCriteria criteria = new QueryCriteria();
		
		Volume v1 = (new VolumeBuilder()).withId( "0123456" ).withName( "Awesome" ).build();
		Volume v2 = (new VolumeBuilder()).withId( "7890" ).withName( "Awesome 2" ).build();
		
		List<Volume> contexts = new ArrayList<Volume>();
		contexts.add( v1 );
		contexts.add( v2 );
		
		DateRange range = new DateRange();
		range.setStart( 0L );
		range.setEnd( 123456789L );
		
		criteria.setContexts( contexts );
		criteria.setRange( range );
		
		String result = influxRepo.formulateQueryString( criteria );
		
		String expectation = "select * from volume_metrics where ( time > 0s and time < 123456789s ) " +
			"and ( volume_id = '0123456' or volume_id = '7890' )";
		
		System.out.println( "Expected: " + expectation );
		System.out.println( "Actual: " + result );
		
		assertEquals( "Query string is incorrect.", result, expectation );
	}
	
	@Test
	public void testFindAllQuery(){
		
		char[] creds = "password".toCharArray();
		InfluxMetricRepository influxRepo = new InfluxMetricRepository( null, null, creds );
		
		System.out.println( "Testing a complicated query very much like what we receive from the UI." );
		
		QueryCriteria criteria = new QueryCriteria();
		
		System.out.println( "Testing the query used for the findAll method." );
		
		String result = influxRepo.formulateQueryString( criteria );
		
		String expectation = "select * from volume_metrics";
			
		System.out.println( "Expected: " + expectation );
		System.out.println( "Actual: " + result );
		
		assertEquals( "Query string is incorrect.", result, expectation );
	}
	
}
