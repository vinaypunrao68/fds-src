package com.formationds.om.repository.influxdb;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.DateRange;
import com.formationds.om.repository.query.QueryCriteria;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;

public class TestInfluxRepository {
    @Test
    public void verifyUIQuerySyntax() {

        char[] creds = "password".toCharArray();
        InfluxMetricRepository influxRepo = new InfluxMetricRepository( null, null, creds );

        System.out.println( "Testing a complicated query very much like what we receive from the UI." );

        QueryCriteria criteria = new QueryCriteria();

        Volume v1 = (new Volume.Builder("Awesome")).id( 123456L ).create();
        Volume v2 = (new Volume.Builder("Awesome 2")).id( 7890L ).create();

        List<Volume> contexts = new ArrayList<Volume>();
        contexts.add( v1 );
        contexts.add( v2 );

        DateRange range = new DateRange();
        range.setStart( 0L );
        range.setEnd( 123456789L );

        criteria.setContexts( contexts );
        criteria.setRange( range );

        String result = influxRepo.formulateQueryString( criteria,
                                                         influxRepo.getVolumeIdColumnName().get(),
                                                         TimeUnit.SECONDS );

        String expectation = "select * from volume_metrics where ( time > 0s and time < 123456789s ) " +
                             "and ( volume_id = '0123456' or volume_id = '7890' )";

        System.out.println( "Expected: " + expectation );
        System.out.println( "Actual: " + result );

        assertEquals( "Query string is incorrect.",  expectation, result );
    }

    @Test
    public void testFindAllQuery() {

        char[] creds = "password".toCharArray();
        InfluxMetricRepository influxRepo = new InfluxMetricRepository( null, null, creds );

        System.out.println( "Testing a complicated query very much like what we receive from the UI." );

        QueryCriteria criteria = new QueryCriteria();

        System.out.println( "Testing the query used for the findAll method." );

        String result = influxRepo.formulateQueryString( criteria,
                                                         influxRepo.getVolumeIdColumnName().get(),
                                                         TimeUnit.SECONDS );

        String expectation = "select * from volume_metrics";

        System.out.println( "Expected: " + expectation );
        System.out.println( "Actual: " + result );

        assertEquals( "Query string is incorrect.", expectation, result );
    }

    @Test
    public void testfindLastFirebreakEventQuery() {
        InfluxEventRepository influxEventRepository = new InfluxEventRepository( null, null, "password".toCharArray() );

        Volume v1 = (new Volume.Builder("Awesome")).id( 123456L ).create();
        Volume v2 = (new Volume.Builder("Awesome 2")).id( 7890L ).create();

        List<Volume> contexts = new ArrayList<Volume>();
        contexts.add( v1 );
        contexts.add( v2 );

//        Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
//        Long tsOneDayAgo = oneDayAgo.toEpochMilli();
// For this test, use a fixed time
        Long tsOneDayAgo = 12345678901L;

        QueryCriteria criteria = new QueryCriteria( new DateRange( tsOneDayAgo ) );
        criteria.setContexts( contexts );

        String result = influxEventRepository.formulateQueryString( criteria,
                                                                    InfluxEventRepository.FBEVENT_VOL_ID_COLUMN_NAME,
                                                                    TimeUnit.MILLISECONDS );
        String expectation = "select * from events where time > " + tsOneDayAgo + "ms" +
                             " and ( " + InfluxEventRepository.FBEVENT_VOL_ID_COLUMN_NAME + " = '0123456'" +
                             " or " +
                             InfluxEventRepository.FBEVENT_VOL_ID_COLUMN_NAME + " = '7890' )";
        System.out.println( "Expected: " + expectation );
        System.out.println( "Actual:   " + result );

        assertEquals( "Query string is incorrect.", expectation, result );


        //                                 findLatestFirebreak( new Volume(0, "1", "domain", "vol1"), FirebreakType.PERFORMANCE );

    }

}
