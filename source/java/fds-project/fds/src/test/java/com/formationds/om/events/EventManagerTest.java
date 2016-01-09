/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.events;

import static org.mockito.Mockito.*;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.MetricRepository;
import com.formationds.util.Configuration;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.AsyncAm;
import com.google.gson.reflect.TypeToken;

import org.junit.*;

import java.lang.reflect.Type;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;
import java.time.Instant;
import java.util.*;

public class EventManagerTest {

    private static final Type TYPE = new TypeToken<List<VolumeDatapoint>>(){}.getType();

    static Object key = new Object();
    static final Set<? super Event> events = new LinkedHashSet<>();

    static final Configuration mockedConfiguration = mock( Configuration.class );
    static final ConfigurationApi mockedConfig = mock( ConfigurationApi.class );
    static final AsyncAm  mockedAMService = mock( AsyncAm.class );
    static final MetricRepository
        metricsRepoMock = mock( MetricRepository.class );

    @BeforeClass
    static public void setUpClass() throws Exception {

        Path fdsRoot = Files.createTempDirectory( "fds" );
        Path vardb = Files.createDirectories( fdsRoot.resolve( "var/db" ) );
        when( mockedConfiguration.getFdsRoot() ).thenReturn( fdsRoot.toString() );

        SingletonConfiguration.instance().setConfig( mockedConfiguration );
        System.setProperty( "fds-root", SingletonConfiguration.instance().getConfig().getFdsRoot() );

        SingletonConfigAPI.instance().api( mockedConfig );
        SingletonAmAPI.instance().api( mockedAMService );
        VolumeStatus vstat = new VolumeStatus();
        vstat.setCurrentUsageInBytes( 1024 );
        Optional<VolumeStatus> opStats = Optional.of( vstat );
        when( metricsRepoMock.getLatestVolumeStatus( "" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "volume1" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "volume2" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "volume3" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "sys-ov1" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "sys-bv1" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "u1-ov1" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "u1-bv1" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "u2-ov1" ) ).thenReturn( opStats );
        when( metricsRepoMock.getLatestVolumeStatus( "u2.bv1" ) ).thenReturn( opStats );

        Files.deleteIfExists( Paths.get( SingletonConfiguration.instance().getConfig().getFdsRoot(), "var", "db",
                                         "events.odb" ) );
        Files.deleteIfExists( Paths.get( SingletonConfiguration.instance().getConfig().getFdsRoot(), "var", "db",
                                         "events.odb$" ) );

        // initialize the event manager notification handler to store in an in-memory map
        EventManager.instance().initEventNotifier( key, ( e ) -> {
            events.add( e );
            return true;
        });
    }

    @Before
    public void setUp() throws Exception { }

    @After
    public void tearDown() {}

    /**
     * This tests an issue in Calculation.isFirebreak() which previously resulted in
     * it returning true when the long-term capacity was zero.  Dividing a double by
     * zero results in it returning a value representing Infinity (which is of course
     * greater than the threshold).
     */
    @Test
    public void testCalculationIsFirebreak() {
        double r = 6.164390352038617D / 0.0D;
        Assert.assertFalse( Double.isNaN( r ) );
        Assert.assertTrue( Double.isInfinite( r ) );

        Assert.assertFalse( Calculation.isFirebreak( 6.164390352038617D, 0.0D ) );

        Assert.assertTrue( Calculation.isFirebreak( 15.0D, 3.0D ) );
        Assert.assertFalse( Calculation.isFirebreak( 2.0D, 1.0D ) );
    }

    // NOTE: this used to test firebreak events through the repository.  That has a dependency on
    // the external database running.  This has been modified to only test that the event notification mechanism works
    @Test
    public void testEventNotification() {
        clearEvents();

        Volume v1 = new VolumeBuilder().withId( "1" ).withName( "v1" ).build();
        Volume v2 = new VolumeBuilder().withId( "2" ).withName( "v2" ).build();

        long nowMS = Instant.now().toEpochMilli();
        long oldTsMS = Instant.now().minus( Duration.ofHours( 25 ) ).toEpochMilli();
        final FirebreakEvent v1e1 = new FirebreakEvent( v1, FirebreakType.CAPACITY, nowMS - 120000, 0x7FFFL, 5.00D );
        final FirebreakEvent v2e1 = new FirebreakEvent( v2, FirebreakType.CAPACITY, nowMS - 120000, 0xFFFFL, 10.00D );
        final FirebreakEvent v1e2 = new FirebreakEvent( v1, FirebreakType.CAPACITY, nowMS, 0x7FFFL, 5.00D );
        final FirebreakEvent v2e2 = new FirebreakEvent( v2, FirebreakType.CAPACITY, nowMS, 0xFFFFL, 10.00D );
        final FirebreakEvent ov1e1 = new FirebreakEvent( v1, FirebreakType.CAPACITY, oldTsMS, 0x7FFFL, 5.00D );
        final FirebreakEvent ov2e1 = new FirebreakEvent( v2, FirebreakType.CAPACITY, oldTsMS, 0xFFFFL, 10.00D );

        // perf events
        final FirebreakEvent v1pe1 = new FirebreakEvent( v1, FirebreakType.PERFORMANCE, nowMS-360000, 0x7FFFL, 50.00D );
        final FirebreakEvent v2pe1 = new FirebreakEvent( v2, FirebreakType.PERFORMANCE, nowMS-360000, 0xFFFFL, 100.00D );

        EventManager.instance().notifyEvent( v1e1 );
        Assert.assertTrue( events.contains( v1e1 ) );

        EventManager.instance().notifyEvent( v2e1 );
        Assert.assertTrue( events.contains( v2e1 ) );

        EventManager.instance().notifyEvent( v1e2 );
        Assert.assertTrue( events.contains( v1e2 ) );

        EventManager.instance().notifyEvent( v2e2 );
        Assert.assertTrue( events.contains( v2e2 ) );

        EventManager.instance().notifyEvent( ov1e1 );
        Assert.assertTrue( events.contains( ov1e1 ) );

        EventManager.instance().notifyEvent( ov2e1 );
        Assert.assertTrue( events.contains( ov2e1 ) );

        EventManager.instance().notifyEvent( v1pe1 );
        Assert.assertTrue( events.contains( v1pe1 ) );

        EventManager.instance().notifyEvent( v2pe1 );
        Assert.assertTrue( events.contains( v2pe1 ) );
    }

    private void clearEvents() {
        events.clear();
    }
}
