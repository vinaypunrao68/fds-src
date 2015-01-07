/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.events;

import static org.mockito.Mockito.*;

import com.formationds.apis.AmService;
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
import com.formationds.om.repository.EventRepository;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.util.Configuration;
import com.formationds.xdi.ConfigurationApi;
import com.google.gson.reflect.TypeToken;

import org.apache.thrift.TException;
import org.junit.*;

import java.lang.reflect.Type;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;
import java.time.Instant;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

public class EventManagerTest {

    private static final Type TYPE = new TypeToken<List<VolumeDatapoint>>(){}.getType();

    static Object key = new Object();
    static final Set<? super Event> events = new LinkedHashSet<>();

    static final Configuration mockedConfiguration = mock(Configuration.class);
    static final ConfigurationApi mockedConfig = mock(ConfigurationApi.class);
    static final AmService.Iface mockedAMService = mock(AmService.Iface.class);
    static final MetricsRepository metricsRepoMock = mock( MetricsRepository.class );

    @BeforeClass
    static public void setUpClass() throws Exception {

        Path fdsRoot = Files.createTempDirectory("fds");
        Path vardb = Files.createDirectories(fdsRoot.resolve("var/db"));
        when(mockedConfiguration.getFdsRoot()).thenReturn(fdsRoot.toString());

        SingletonConfiguration.instance().setConfig(mockedConfiguration);
        System.setProperty("fds-root", SingletonConfiguration.instance().getConfig().getFdsRoot() );

        SingletonConfigAPI.instance().api(mockedConfig);
        SingletonAmAPI.instance().api(mockedAMService);
        VolumeStatus vstat = new VolumeStatus();
        vstat.setCurrentUsageInBytes(1024);
        Optional<VolumeStatus> opStats = Optional.of( vstat );
        when(metricsRepoMock.getLatestVolumeStatus("")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("volume1")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("volume2")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("volume3")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("sys-ov1")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("sys-bv1")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("u1-ov1")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("u1-bv1")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("u2-ov1")).thenReturn(opStats);
        when(metricsRepoMock.getLatestVolumeStatus("u2.bv1")).thenReturn(opStats);

        Files.deleteIfExists(Paths.get(SingletonConfiguration.instance().getConfig().getFdsRoot(), "var", "db", "events.odb"));
        Files.deleteIfExists(Paths.get(SingletonConfiguration.instance().getConfig().getFdsRoot(), "var", "db", "events.odb$"));

        // initialize the event manager notification handler to store in both the event repository and an in-memory map
        EventManager.instance().initEventNotifier(key, (e) -> {
            SingletonRepositoryManager.instance().getEventRepository().save(e);
            events.add(e);
            return true;
        });
    }

    @Before
    public void setUp() throws Exception { }

    @After
    public void tearDown() {}

    private Map<String, Integer> volids = new ConcurrentHashMap<>();
    private AtomicInteger        volcnt = new AtomicInteger(1);

    /**
     * @return a copy of the test data set with timestamps set offset from 24 hours ago
     */
    public static VolumeDatapoint[][] getTestDataSet() {
        return getTestDataSet(Instant.now().toEpochMilli() - TimeUnit.HOURS.toMillis(24), 7200);
    }

    /**
     *
     * @param ts the starting timestamp
     * @param tsoffset the timestamp offset for each collection of data.
     * @return a copy of the test data set with timestamps offset from the specified starting time.
     */
    public static VolumeDatapoint[][] getTestDataSet(long ts, int tsoffset) {
        VolumeDatapoint[][] c = new VolumeDatapoint[testdps.length][];
        for (int i = 0; i < testdps.length; i++) {
            c[i] = testdps[i].clone();
            for (int j = 0; j < c[i].length; j++) {
                VolumeDatapoint dp = c[i][j];
                dp.setTimestamp(ts + (i * tsoffset));
            }
        }
        return c;
    }

    // set of datapoints to generate events.  These were captured from real data.  Note
    // that the timestamp values are overwritten when processed to set them to current time
    // minus an offset;
    // NOTE: this dataset is currently used by other tests (ie FirebreakHelperTest).  If it changes, those tests may need to be updated.
    private static final VolumeDatapoint[][] testdps =
    new VolumeDatapoint[][]
    {
    {
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Puts", 11.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Logical Bytes", 1690657.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Physical Bytes", 1690657.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Metadata Bytes", 179928.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Blobs", 2499.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Objects", 2499.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Ave Blob Size", 676.5334133653462D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D),
        // Capacity Firebreak
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Short Term Capacity Sigma", 16.43837427210252D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Long Term Capacity Sigma", 4.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Short Term Capacity WMA", 30193.157894736843D),
        // performance firebreak
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Short Term Perf Sigma", 15.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Long Term Perf Sigma", 3.0D),
        new VolumeDatapoint(1416588552440L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D),
    }, {
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Logical Bytes", 1696802.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Physical Bytes", 1696802.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Metadata Bytes", 180792.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Blobs", 2511.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Objects", 2511.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Ave Blob Size", 675.747510951812D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Short Term Capacity Sigma", 16.43837427210252D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Short Term Capacity WMA", 30193.157894736843D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552455L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D),
    }, {
        new VolumeDatapoint(1416588552456L, "5", "u2-ov1", "Puts", 11.0D),
        new VolumeDatapoint(1416588552456L, "5", "u2-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552457L, "5", "u2-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552457L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552457L, "5", "u2-ov1", "Logical Bytes", 1690222.0D),
        new VolumeDatapoint(1416588552457L, "5", "u2-ov1", "Physical Bytes", 1690222.0D),
        new VolumeDatapoint(1416588552457L, "5", "u2-ov1", "Metadata Bytes", 179496.0D),
        new VolumeDatapoint(1416588552457L, "5", "u2-ov1", "Blobs", 2493.0D),
        new VolumeDatapoint(1416588552457L, "5", "u2-ov1", "Objects", 2493.0D),
        new VolumeDatapoint(1416588552458L, "5", "u2-ov1", "Ave Blob Size", 677.9871640593663D),
        new VolumeDatapoint(1416588552458L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552458L, "5", "u2-ov1", "Short Term Capacity Sigma", 7.705487940048173D),
        new VolumeDatapoint(1416588552458L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Short Term Capacity WMA", 30194.894736842107D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D),
    },{
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Logical Bytes", 1696369.0D),
        new VolumeDatapoint(1416588552459L, "5", "u2-ov1", "Physical Bytes", 1696369.0D),
        new VolumeDatapoint(1416588552460L, "5", "u2-ov1", "Metadata Bytes", 180360.0D),
        new VolumeDatapoint(1416588552460L, "5", "u2-ov1", "Blobs", 2505.0D),
        new VolumeDatapoint(1416588552460L, "5", "u2-ov1", "Objects", 2505.0D),
        new VolumeDatapoint(1416588552460L, "5", "u2-ov1", "Ave Blob Size", 677.1932135728543D),
        new VolumeDatapoint(1416588552460L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552460L, "5", "u2-ov1", "Short Term Capacity Sigma", 7.705487940048173D),
        new VolumeDatapoint(1416588552460L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552461L, "5", "u2-ov1", "Short Term Capacity WMA", 30194.894736842107D),
        new VolumeDatapoint(1416588552461L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552461L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552461L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D)
    },{
        new VolumeDatapoint(1416588552657L, "3", "u1-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552657L, "3", "u1-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552657L, "3", "u1-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552657L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552657L, "3", "u1-ov1", "Logical Bytes", 1702939.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Physical Bytes", 1702939.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Metadata Bytes", 181656.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Blobs", 2523.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Objects", 2523.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Ave Blob Size", 674.9659135949266D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Short Term Capacity Sigma", 16.43837427210252D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Short Term Capacity WMA", 30193.157894736843D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552658L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D),
    },{

        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Logical Bytes", 1709089.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Physical Bytes", 1709089.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Metadata Bytes", 182520.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Blobs", 2535.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Objects", 2535.0D),
        new VolumeDatapoint(1416588552659L, "3", "u1-ov1", "Ave Blob Size", 674.1968441814596D),
        new VolumeDatapoint(1416588552660L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552660L, "3", "u1-ov1", "Short Term Capacity Sigma", 16.43837427210252D),
        new VolumeDatapoint(1416588552660L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552660L, "3", "u1-ov1", "Short Term Capacity WMA", 30193.157894736843D),
        new VolumeDatapoint(1416588552660L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552660L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552660L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D),
    },{

        new VolumeDatapoint(1416588552660L, "5", "u2-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552660L, "5", "u2-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Logical Bytes", 1702516.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Physical Bytes", 1702516.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Metadata Bytes", 181224.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Blobs", 2517.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Objects", 2517.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Ave Blob Size", 676.4068335319826D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552661L, "5", "u2-ov1", "Short Term Capacity Sigma", 7.705487940048173D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Short Term Capacity WMA", 30194.894736842107D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D),
    },{
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Logical Bytes", 1708664.0D),
        new VolumeDatapoint(1416588552662L, "5", "u2-ov1", "Physical Bytes", 1708664.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Metadata Bytes", 182088.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Blobs", 2529.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Objects", 2529.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Ave Blob Size", 675.628311585607D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Short Term Capacity Sigma", 7.705487940048173D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Short Term Capacity WMA", 30194.894736842107D),
        // perf
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Short Term Perf Sigma", 30.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Long Term Perf Sigma", 3.0D),
        new VolumeDatapoint(1416588552663L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D),
    },{
        new VolumeDatapoint(1416588552709L, "3", "u1-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552709L, "3", "u1-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552709L, "3", "u1-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552709L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552709L, "3", "u1-ov1", "Logical Bytes", 1715231.0D),
        new VolumeDatapoint(1416588552709L, "3", "u1-ov1", "Physical Bytes", 1715231.0D),
        new VolumeDatapoint(1416588552709L, "3", "u1-ov1", "Metadata Bytes", 183384.0D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Blobs", 2547.0D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Objects", 2547.0D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Ave Blob Size", 673.4318806438948D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Short Term Capacity Sigma", 16.43837427210252D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Short Term Capacity WMA", 30193.157894736843D),
        new VolumeDatapoint(1416588552710L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D),
    },{
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Puts", 11.0D),
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Logical Bytes", 1720866.0D),
        new VolumeDatapoint(1416588552711L, "3", "u1-ov1", "Physical Bytes", 1720866.0D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Metadata Bytes", 184176.0D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Blobs", 2558.0D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Objects", 2558.0D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Ave Blob Size", 672.73885848319D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Short Term Capacity Sigma", 16.43837427210252D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Short Term Capacity WMA", 30193.157894736843D),
        new VolumeDatapoint(1416588552712L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552713L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552713L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D),
    },{
        new VolumeDatapoint(1416588552713L, "5", "u2-ov1", "Puts", 12.0D),
        new VolumeDatapoint(1416588552713L, "5", "u2-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552713L, "5", "u2-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552713L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552713L, "5", "u2-ov1", "Logical Bytes", 1714807.0D),
        new VolumeDatapoint(1416588552713L, "5", "u2-ov1", "Physical Bytes", 1714807.0D),
        new VolumeDatapoint(1416588552713L, "5", "u2-ov1", "Metadata Bytes", 182952.0D),
        new VolumeDatapoint(1416588552714L, "5", "u2-ov1", "Blobs", 2541.0D),
        new VolumeDatapoint(1416588552714L, "5", "u2-ov1", "Objects", 2541.0D),
        new VolumeDatapoint(1416588552714L, "5", "u2-ov1", "Ave Blob Size", 674.8551751279024D),
        new VolumeDatapoint(1416588552714L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D),
        new VolumeDatapoint(1416588552714L, "5", "u2-ov1", "Short Term Capacity Sigma", 7.705487940048173D),
        new VolumeDatapoint(1416588552714L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D),
        new VolumeDatapoint(1416588552714L, "5", "u2-ov1", "Short Term Capacity WMA", 30194.894736842107D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D),
    },{
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Puts", 11.0D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Gets", 0.0D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Queue Full", 0.0D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Logical Bytes", 1720450.0D),
        new VolumeDatapoint(1416588552715L, "5", "u2-ov1", "Physical Bytes", 1720450.0D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Metadata Bytes", 183744.0D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Blobs", 2552.0D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Objects", 2552.0D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Ave Blob Size", 674.1575235109718D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D),
        // capacity fb
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Short Term Capacity Sigma", 7.705487940048173D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Long Term Capacity Sigma", 1.0D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Short Term Capacity WMA", 30194.894736842107D),
        new VolumeDatapoint(1416588552716L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552717L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D),
        new VolumeDatapoint(1416588552717L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D),
    }
    };

    /**
     * This tests an issue in Calculation.isFirebreak() which previously resulted in
     * it returning true when the long-term capacity was zero.  Dividing a double by
     * zero results in it returning a value representing Infinity (which is of course
     * greater than the threshold).
     */
    @Test
    public void testCalculationIsFirebreak() {
        double r = 6.164390352038617D / 0.0D;
        Assert.assertFalse(Double.isNaN(r));
        Assert.assertTrue(Double.isInfinite(r));

        Assert.assertFalse(Calculation.isFirebreak(6.164390352038617D, 0.0D));
    }

    @Test
    public void testFirebreakEvents() {

        VolumeDatapointEntityPersistListener vdpl = new VolumeDatapointEntityPersistListener();
        final long now = System.currentTimeMillis();
        final AtomicInteger c = new AtomicInteger(testdps.length+2);
        Arrays.asList(testdps).forEach((u) -> {
//            System.out.println(u);
            int i = c.decrementAndGet();
            final List<VolumeDatapoint> volumeDatapoints = Arrays.asList(u);
            volumeDatapoints.forEach((vdp) -> { vdp.setTimestamp(now - (i * 7200)); });

            int s = events.size();
            try {
//                System.out.println(String.format("// %s datapoints", volumeDatapoints.size()));
                vdpl.doPostPersist(volumeDatapoints);
            } catch (TException e) {
                Assert.fail("Failed to process volume datapoints for firebreak events.  " +
                            "Check the Mock setup of the AMService.  Error: " + e);

            }
//            System.out.println(String.format("// Total events %d: ", events.size()));
//            events.forEach((e) -> System.out.println("// IME: " + e));
        });

//        SingletonRepositoryManager.instance()
//                                  .getEventRepository()
//                                  .findAll()
//                                  .forEach((e) -> System.out.println("Event: " + e));

        final Volume v3 = new VolumeBuilder().withId("3").withName("u1-ov1").build();
        FirebreakEvent fbe = new EventRepository().findLatestFirebreak(v3, FirebreakType.CAPACITY);
        FirebreakEvent fbpe = new EventRepository().findLatestFirebreak(v3, FirebreakType.PERFORMANCE);

        final Volume v5 = new VolumeBuilder().withId("5").withName("u2-ov1").build();
        FirebreakEvent fbe2 = new EventRepository().findLatestFirebreak(v5, FirebreakType.CAPACITY);
        FirebreakEvent fbpe2 = new EventRepository().findLatestFirebreak(v5, FirebreakType.PERFORMANCE);

//        if (fbe != null) System.out.println("Latest FBE(u1-ov1): " + fbe);
//        if (fbpe != null) System.out.println("Latest FBPE(u1-ov1): " + fbpe);
//        if (fbe2 != null) System.out.println("Latest FBE(u2-ov1): " + fbe2);
//        if (fbpe2 != null) System.out.println("Latest FBPE(u2-ov1): " + fbpe2);
//
        Assert.assertTrue(v3.getName().equals(fbe.getVolumeName()));
        Assert.assertTrue(fbe.getFirebreakType().equals(FirebreakType.CAPACITY));
        Assert.assertTrue(fbe.getSigma() > 0.0D);

        Assert.assertTrue(v3.getName().equals(fbpe.getVolumeName()));
        Assert.assertTrue(fbpe.getFirebreakType().equals(FirebreakType.PERFORMANCE));
        Assert.assertTrue(fbpe.getSigma() == 15.0D);

        Assert.assertTrue(v5.getName().equals(fbe2.getVolumeName()));
        Assert.assertTrue(fbe2.getFirebreakType().equals(FirebreakType.CAPACITY));
        Assert.assertTrue(fbe2.getSigma() > 0.0D);

        Assert.assertTrue(v5.getName().equals(fbpe2.getVolumeName()));
        Assert.assertTrue(fbpe2.getFirebreakType().equals(FirebreakType.PERFORMANCE));
        Assert.assertTrue(fbpe2.getSigma() > 0.0D);

    }

    @Test
    public void testFindLatestFirebreakNewRepo() {
        EventRepository er = SingletonRepositoryManager.instance().getEventRepository();
        clearEvents();

        Volume v1 = new VolumeBuilder().withId("1").withName("v1").build();
        Volume v2 = new VolumeBuilder().withId("2").withName("v2").build();

        long nowMS = Instant.now().toEpochMilli();
        long oldTsMS = Instant.now().minus(Duration.ofHours(25)).toEpochMilli();
        final FirebreakEvent v1e1 = new FirebreakEvent(v1, FirebreakType.CAPACITY, nowMS - 120000, 0x7FFFL, 5.00D);
        final FirebreakEvent v2e1 = new FirebreakEvent(v2, FirebreakType.CAPACITY, nowMS - 120000, 0xFFFFL, 10.00D);
        final FirebreakEvent v1e2 = new FirebreakEvent(v1, FirebreakType.CAPACITY, nowMS, 0x7FFFL, 5.00D);
        final FirebreakEvent v2e2 = new FirebreakEvent(v2, FirebreakType.CAPACITY, nowMS, 0xFFFFL, 10.00D);
        final FirebreakEvent ov1e1 = new FirebreakEvent(v1, FirebreakType.CAPACITY, oldTsMS, 0x7FFFL, 5.00D);
        final FirebreakEvent ov2e1 = new FirebreakEvent(v2, FirebreakType.CAPACITY, oldTsMS, 0xFFFFL, 10.00D);

        // perf events
        final FirebreakEvent v1pe1 = new FirebreakEvent(v1, FirebreakType.PERFORMANCE, nowMS-360000, 0x7FFFL, 50.00D);
        final FirebreakEvent v2pe1 = new FirebreakEvent(v2, FirebreakType.PERFORMANCE, nowMS-360000, 0xFFFFL, 100.00D);

        EventManager.instance().notifyEvent(v1e1);
        EventManager.instance().notifyEvent(v2e1);
        EventManager.instance().notifyEvent(v1e2);
        EventManager.instance().notifyEvent(v2e2);
        EventManager.instance().notifyEvent(ov1e1);
        EventManager.instance().notifyEvent(ov2e1);
        EventManager.instance().notifyEvent(v1pe1);
        EventManager.instance().notifyEvent(v2pe1);

//        System.out.println("now: " + nowMS + ";2 mins ago: " + (nowMS-120000) + "; 25 hours ago: " + oldTsMS + "; diff(ms): " + (nowMS-oldTsMS));
//        er.findAll().forEach((e) -> System.out.println("Event: " + e));

        FirebreakEvent fbe = new EventRepository().findLatestFirebreak(v1, FirebreakType.CAPACITY);
        FirebreakEvent fbe2 = new EventRepository().findLatestFirebreak(v2, FirebreakType.CAPACITY);

//        if (fbe != null) System.out.println("Latest FBE(v1): " + fbe);
//        if (fbe2 != null) System.out.println("Latest FBE(v2): " + fbe2);

        Assert.assertTrue(v1e2.equals(fbe));
        Assert.assertTrue(v2e2.equals(fbe2));

        FirebreakEvent fbv1pe1 = er.findLatestFirebreak(v1, FirebreakType.PERFORMANCE);
        FirebreakEvent fbv2pe1 = er.findLatestFirebreak(v2, FirebreakType.PERFORMANCE);
        Assert.assertTrue(v1pe1.equals(fbv1pe1));
        Assert.assertTrue(v2pe1.equals(fbv2pe1));
    }

    private void clearEvents() {
        EventRepository er = SingletonRepositoryManager.instance().getEventRepository();
        er.findAll().forEach((e) -> er.delete(e));
    }
}
