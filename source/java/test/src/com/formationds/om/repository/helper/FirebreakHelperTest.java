package com.formationds.om.repository.helper;

import com.formationds.apis.AmService;
import com.formationds.apis.VolumeStatus;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.EventManagerTest;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.util.Configuration;
import com.formationds.xdi.ConfigurationApi;
import org.junit.*;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.time.Instant;
import java.util.*;
import java.util.stream.Collectors;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/** 
 * FirebreakHelper Tester.
 *
 */
// TODO: this test is currently using data from EventManagerTest.  Use data generator once available.
public class FirebreakHelperTest {

	static final ConfigurationApi mockedConfigApi    = mock(ConfigurationApi.class);
    static final Configuration mockedConfig    = mock(Configuration.class);
    static final AmService.Iface  mockedAMService = mock(AmService.Iface.class);

    @BeforeClass
    static public void setUpClass() throws Exception {
        SingletonConfigAPI.instance().api(mockedConfigApi);
        SingletonConfiguration.instance().setConfig(mockedConfig);
        SingletonAmAPI.instance().api(mockedAMService);
        VolumeStatus vstat = new VolumeStatus();
        vstat.setCurrentUsageInBytes(1024);
        when(mockedAMService.volumeStatus("", "u1-ov1")).thenReturn(vstat);
        when(mockedAMService.volumeStatus("", "u2-ov1")).thenReturn(vstat);
    }

    @Before
    public void before() throws Exception { 
    } 

    @After
    public void after() throws Exception { 
    } 

    /**
     * Method: processFirebreak(final List<VolumeDatapoint> queryResults) 
     */ 
    @Test
    @Ignore //not implemented
    public void testProcessFirebreak() throws Exception { 
        //TODO: Test goes here... 
    } 

    /**
     * Method: findFirebreak(final List<VolumeDatapoint> datapoints) 
     */ 
    @Test
    public void testFindFirebreak() throws Exception {
        VolumeDatapoint[][] rawdata = EventManagerTest.getTestDataSet();
        List<VolumeDatapoint> all = new ArrayList<>();
        Arrays.asList(rawdata).forEach(((r) -> all.addAll(Arrays.asList(r))));

        FirebreakHelper fbh = new FirebreakHelper();
        Map<String, FirebreakHelper.VolumeDatapointPair> m = fbh.findFirebreak(all);

        Assert.assertEquals(2, m.size());
        String v1 = "3";
        String v2 = "5";
        Assert.assertNotNull(m.get(v1));
        Assert.assertNotNull(m.get(v2));
    }

    /**
     * Method: findFirebreakEvents(final List<VolumeDatapoint> datapoints) 
     */ 
    @Test
    public void testFindFirebreakEvents() throws Exception {
        VolumeDatapoint[][] rawdata = EventManagerTest.getTestDataSet();
        List<VolumeDatapoint> all = new ArrayList<>();
        Arrays.asList(rawdata).forEach(((r) -> all.addAll(Arrays.asList(r))));

        FirebreakHelper fbh = new FirebreakHelper();
        Map<String, EnumMap<FirebreakType,FirebreakHelper.VolumeDatapointPair>> m = fbh.findFirebreakEvents(all);

        //
        Assert.assertEquals(2, m.size());
        String v1 = "3";
        String v2 = "5";
        Assert.assertNotNull(m.get(v1));
        Assert.assertNotNull(m.get(v2));

        Assert.assertEquals(2, m.get(v1).size());
        Assert.assertEquals(2, m.get(v2).size());
    }

    /**
     * Method: extractPairs(List<VolumeDatapoint> datapoints) 
     */ 
    @Test
    public void testExtractPairs() throws Exception {
        VolumeDatapoint[][] rawdata = EventManagerTest.getTestDataSet();
        List<VolumeDatapoint[]> rawdatal = Arrays.asList(rawdata);
        rawdatal.forEach((r) -> {
            // Assumptions here that the raw data from EventManagerTest only contains data for a
            // single volume at a time.  That is not necessarily true of data from the stats stream
            List<FirebreakHelper.VolumeDatapointPair> vdps = new FirebreakHelper().extractPairs(Arrays.asList(r));
            Assert.assertTrue("test data has 2 firebreak datapoint pairs each", vdps.size() == 2);
            Assert.assertEquals(FirebreakType.CAPACITY, vdps.get(0).getFirebreakType());
            Assert.assertEquals(FirebreakType.PERFORMANCE, vdps.get(1).getFirebreakType());
        });

        // this time we combine all of the data across timestamps and volumes.
        List<VolumeDatapoint> all = new ArrayList<>();
        rawdatal.forEach((r) -> Arrays.asList(r).forEach(all::add));
        List<FirebreakHelper.VolumeDatapointPair> vdps = new FirebreakHelper().extractPairs(all);

        // data set has 12 entries each with 2 pairs so we expect a total of 24 pairs back
        Assert.assertTrue(vdps.size() == (2*rawdatal.size()));

        // this third tests is a negative test to make sure we handle
        // 0 entries
        // odd number of entries
        // non-firebreak entries
        List<VolumeDatapoint> neg = Arrays.asList(rawdatal.get(0));
        List<VolumeDatapoint> missingSTC = neg.stream()
                                              .filter((v) -> !Metrics.STC_SIGMA.equals(Metrics.byMetadataKey(v.getKey())))
                                              .collect(Collectors.toList());

        // currently the implementation will not return anything for unpaired sigmas.  It does not
        // report any errors either.
        List<FirebreakHelper.VolumeDatapointPair> missingSTCPairs = new FirebreakHelper().extractPairs(missingSTC);
        Assert.assertTrue(missingSTCPairs.size() == 1);
        Assert.assertTrue(missingSTCPairs.get(0).getFirebreakType().equals(FirebreakType.PERFORMANCE));

        List<FirebreakHelper.VolumeDatapointPair> empty = new FirebreakHelper().extractPairs(new ArrayList<>());
        Assert.assertTrue(empty.isEmpty());
    }

    /**
     * Method: extractFirebreakDatapoints(List<VolumeDatapoint> datapoints) 
     */ 
    @Test
    public void testExtractFirebreakDatapoints() throws Exception {
        VolumeDatapoint[][] rawdata = EventManagerTest.getTestDataSet();
        List<VolumeDatapoint[]> rawdatal = Arrays.asList(rawdata);
        rawdatal.forEach((r) -> {
            // Assumptions here that the raw data from EventManagerTest only contains data for a
            // single volume at a time.  That is not necessarily true of data from the stats stream
            List<VolumeDatapoint> vdps = new FirebreakHelper().extractFirebreakDatapoints(Arrays.asList(r));
            Assert.assertTrue("test data has 4 firebreak points each", vdps.size() == 4);

            Assert.assertTrue(vdps.get(0).getKey().equals(Metrics.STC_SIGMA.key()));
            Assert.assertTrue(vdps.get(1).getKey().equals(Metrics.LTC_SIGMA.key()));
            Assert.assertTrue(vdps.get(2).getKey().equals(Metrics.STP_SIGMA.key()));
            Assert.assertTrue(vdps.get(3).getKey().equals(Metrics.LTP_SIGMA.key()));
        });

        // this one first strips the firebreak metrics from the lists and then ensures that
        // extractFirebreakDatapoints returns an empty list.
        rawdatal.forEach((r) -> {
            List<VolumeDatapoint> stripped = Arrays.asList(r)
                                                   .stream()
                                                   .filter((v) -> !v.getKey().contains("Sigma"))
                                                   .collect(Collectors.toList());

            List<VolumeDatapoint> fbdps = new FirebreakHelper().extractFirebreakDatapoints(stripped);
            Assert.assertTrue(fbdps.isEmpty());
        });
    }

    /**
     * Method: isFirebreak(VolumeDatapointPair p) 
     */ 
    @Test
    public void testIsFirebreak() throws Exception {
        FirebreakHelper fbh = new FirebreakHelper();
        Long ts = Instant.now().toEpochMilli();
        FirebreakHelper.VolumeDatapointPair cap1 = newVDPair(ts, FirebreakType.CAPACITY, 0.0D, 0.0D);
        FirebreakHelper.VolumeDatapointPair cap2 = newVDPair(ts, FirebreakType.CAPACITY, 0.0D, 10.0D);
        FirebreakHelper.VolumeDatapointPair cap3 = newVDPair(ts, FirebreakType.CAPACITY, 10.0D, 0.0D);
        FirebreakHelper.VolumeDatapointPair cap4 = newVDPair(ts, FirebreakType.CAPACITY, 12.0D, 4.0D);
        FirebreakHelper.VolumeDatapointPair cap5 = newVDPair(ts, FirebreakType.CAPACITY, 12.0D, 3.0D);
        Assert.assertFalse(fbh.isFirebreak(cap1));
        Assert.assertFalse(fbh.isFirebreak(cap2));
        Assert.assertFalse(fbh.isFirebreak(cap3));
        Assert.assertFalse(fbh.isFirebreak(cap4));
        Assert.assertTrue(fbh.isFirebreak(cap5));

        FirebreakHelper.VolumeDatapointPair p1 = newVDPair(ts, FirebreakType.PERFORMANCE, 0.0D, 0.0D);
        FirebreakHelper.VolumeDatapointPair p2 = newVDPair(ts, FirebreakType.PERFORMANCE, 0.0D, 100.0D);
        FirebreakHelper.VolumeDatapointPair p3 = newVDPair(ts, FirebreakType.PERFORMANCE, 100.0D, 0.0D);
        FirebreakHelper.VolumeDatapointPair p4 = newVDPair(ts, FirebreakType.PERFORMANCE, 120.0D, 40.0D);
        FirebreakHelper.VolumeDatapointPair p5 = newVDPair(ts, FirebreakType.PERFORMANCE, 120.0D, 30.0D);
        Assert.assertFalse(fbh.isFirebreak(p1));
        Assert.assertFalse(fbh.isFirebreak(p2));
        Assert.assertFalse(fbh.isFirebreak(p3));
        Assert.assertFalse(fbh.isFirebreak(p4));
        Assert.assertTrue(fbh.isFirebreak(p5));
    }

    private FirebreakHelper.VolumeDatapointPair newVDPair(Long ts, FirebreakType t, double st, double lt) {
        VolumeDatapoint stp = new VolumeDatapoint(ts, "1", "v1",
                                                  (FirebreakType.CAPACITY.equals(t) ? Metrics.STC_SIGMA.key() : Metrics.STP_SIGMA.key()),
                                                  st);
        VolumeDatapoint ltp = new VolumeDatapoint(ts, "1", "v1",
                                                  (FirebreakType.CAPACITY.equals(t) ? Metrics.LTC_SIGMA.key() : Metrics.LTP_SIGMA.key()),
                                                  lt);
        return new FirebreakHelper.VolumeDatapointPair(stp, ltp);
    }
}

