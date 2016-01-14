/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.influxdb.InfluxMetricRepository;
import com.formationds.om.repository.influxdb.InfluxRepository;
import com.formationds.om.repository.query.QueryCriteria;
import org.junit.Assert;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Random;
import java.util.TreeMap;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Supplier;

abstract public class RepositoryTestHelper {
  private static final String DB_PATH =
    "/tmp/db/metadata.odb";
  private static final String DB_PATH_TRANS_LOG =
    "/tmp/db/metadata.odb$";

  private static final String JSON =
    "[\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"2.0\",\n" +
      "    \"key\": \"Puts\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"5.0\",\n" +
      "    \"key\": \"Gets\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Queue Full\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Percent of SSD Accesses\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Logical Bytes\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Physical Bytes\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Metadata Bytes\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Blobs\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Objects\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Ave Blob Size\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Ave Objects per Blob\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Capacity Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Long Term Capacity Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Capacity WMA\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Perf Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Long Term Perf Sigma\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "  {\n" +
      "    \"volume\": \"TestVolume\",\n" +
      "    \"value\": \"0.0\",\n" +
      "    \"key\": \"Short Term Perf WMA\",\n" +
      "    \"timestamp\": \"1412126335\"\n" +
      "  },\n" +
      "]";

    /** datapoints generated from the JSON string */
  private static final List<VolumeDatapoint> JSON_DATAPOINTS = new ArrayList<>();

    public static class MetricInfo<T extends Number> implements Supplier<T> {
        private final Supplier<T> supplier;

        // TODO: not currently used... I think only make sense if the supplier is random
        private long min = Long.MIN_VALUE;
        private long max = Long.MAX_VALUE;

        public MetricInfo( Supplier<T> supplier ) {
            this.supplier = supplier;
        }

        public MetricInfo( long min, long max, Supplier<T> supplier ) {
            this.supplier = supplier;
            this.min = min;
            this.max = max;
        }

        @Override
        public T get() {
            return supplier.get();
        }
    }

    private static Random rnd = new Random( System.currentTimeMillis() );

    public static long randomLong() {
        return rnd.nextLong();
    }

    public static double randomDouble() {
        return rnd.nextDouble();
    }

    public static class Sequencer implements Supplier<Long> {
        private final AtomicLong sequence = new AtomicLong( 0 );
        private       int        freq     = 1;
        private final AtomicLong cnt      = new AtomicLong( 0 );

        public Sequencer() {}

        public Sequencer( long start ) { sequence.set( start ); }

        public Sequencer( long start, int freq ) {
            if ( freq < 1 )
                throw new IllegalArgumentException( "frequency must be greater than equal 1" );
            sequence.set( start );
            this.freq = freq;
        }

        public Long get() {
            if ( cnt.incrementAndGet() % freq == 0 )
                return sequence.getAndIncrement();
            else
                return sequence.get();
        }
    }

    public static EnumMap<Metrics, MetricInfo<? extends Number>> metricsSuppliers =
        new EnumMap<Metrics, MetricInfo<? extends Number>>( Metrics.class ) {{

            // TODO: most of these don't make sense with random values... figure out what does make sense...
            put( Metrics.PUTS, new MetricInfo<Long>( new Sequencer( 100 ) ) );
            put( Metrics.GETS, new MetricInfo<Long>( new Sequencer( 0, 10 ) ) );
            // TODO: is this a boolean?
            put( Metrics.QFULL, new MetricInfo<Long>( RepositoryTestHelper::randomLong ) );
            /* @deprecated use {@link #SSD_GETS} */
            put( Metrics.PSSDA, new MetricInfo<Long>( RepositoryTestHelper::randomLong ) );
            put( Metrics.MBYTES, new MetricInfo<Long>( new Sequencer( 1024 * 5, 1000 ) ) );
            put( Metrics.BLOBS, new MetricInfo<Long>( new Sequencer( 2500, 5 ) ) );
            put( Metrics.OBJECTS, new MetricInfo<Long>( new Sequencer( 2500, 5 ) ) );
            put( Metrics.ABS, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            put( Metrics.AOPB, new MetricInfo<Double>( () -> { return 1.0D; } ) );
            // System usage ( sum the most recent of each volume
            put( Metrics.LBYTES, new MetricInfo<Long>( RepositoryTestHelper::randomLong ) );
            put( Metrics.PBYTES, new MetricInfo<Long>( RepositoryTestHelper::randomLong ) );
            put( Metrics.UBYTES, new MetricInfo<Long>( RepositoryTestHelper::randomLong ) );
            // firebreak metrics
            put( Metrics.STC_SIGMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            put( Metrics.LTC_SIGMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            put( Metrics.STP_SIGMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            put( Metrics.LTP_SIGMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            // Performance IOPS
            put( Metrics.STC_WMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            put( Metrics.LTC_WMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            put( Metrics.STP_WMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );
            put( Metrics.LTP_WMA, new MetricInfo<Double>( RepositoryTestHelper::randomDouble ) );

            /** gets from SSD and cache */
            put( Metrics.SSD_GETS, new MetricInfo<Long>( new Sequencer( 0, 5 ) ) );
        }};

    public static Map<Long, List<VolumeDatapoint>> generateDatapoints( Volume volume, int sets ) {
        Map<Long, List<VolumeDatapoint>> tvdps = new TreeMap<>();
        long startTime = Instant.now().toEpochMilli() - (TimeUnit.MINUTES.toMillis( 2 ) * sets);
        for ( int i = 0; i < sets; i++ ) {
            long ts = startTime + (TimeUnit.MINUTES.toMillis( 2 ) * i);
            List<VolumeDatapoint> volumeDatapoints = new ArrayList<>();
            tvdps.put( ts, volumeDatapoints );
            for ( Map.Entry<Metrics, MetricInfo<?>> entry : metricsSuppliers.entrySet() ) {
                VolumeDatapoint vdp = new VolumeDatapoint( ts,
                                                           volume.getId(),
                                                           volume.getName(),
                                                           entry.getKey().name(),
                                                           entry.getValue().get().doubleValue() );
                volumeDatapoints.add( vdp );
            }
        }
        return tvdps;
    }

    public static void save( MetricRepository store, Collection<IVolumeDatapoint> vdps ) {
        store.save( vdps );
    }

    public static void main(String[] args)
    {
        InfluxMetricRepository store = new InfluxMetricRepository( "http://localhost:8086", "root", "root".toCharArray() );
        // TODO: hide this in the repo interface (ie. just make it take the user/creds.  dbname is held by repo)
        Properties props = new Properties();
        props.setProperty( InfluxRepository.CP_DBNAME, InfluxMetricRepository.DEFAULT_METRIC_DB.getName() );
        props.setProperty( InfluxRepository.CP_USER, "root" );
        props.setProperty( InfluxRepository.CP_CRED, "root" );

        store.open( props );
        Volume volume = new Volume( 0, "1", "domain1", "volume1" );
        Map<Long, List<VolumeDatapoint>> dp = generateDatapoints( volume, 10 );

        for ( List<VolumeDatapoint> vdp : dp.values() ) {
            store.save( vdp );
        }

        List<IVolumeDatapoint> results = store.query( new QueryCriteria ());
        Assert.assertEquals( dp.size(), results.size() );
    }

    // TODO: not complete.  query executed but results not validated.
//    public static void query( MetricRepository store,
//                              Map<Long, Collection<IVolumeDatapoint>> expectedResults,
//                              QueryCriteria criteria ) {
//        List<? extends IVolumeDatapoint> results = store.query( criteria );
//        IVolumeDatapoint vdp = results.get( 0 );
//        vdp.getTimestamp();
//    }

// TODO: not a valid unit test, requires running instance of InfluxDB
//    public static class InfluxMetricRepositoryTest {
//
//        @Before
//        public void setUp() {
//            store = new InfluxMetricRepository( "http://localhost:8086", "root", "root".toCharArray() );
//            // TODO: hide this in the repo interface (ie. just make it take the user/creds.  dbname is held by repo)
//            Properties props = new Properties();
//            props.setProperty( InfluxRepository.CP_DBNAME, InfluxMetricRepository.DEFAULT_METRIC_DB.getName() );
//            props.setProperty( InfluxRepository.CP_USER, "root" );
//            props.setProperty( InfluxRepository.CP_CRED, "root" );
//
//            store.open( props );
//        }
//
//        @Test
//        public void testSave() {
//            Volume volume = new Volume( 0, "1", "domain1", "volume1" );
//            Map<Long, List<VolumeDatapoint>> dp = generateDatapoints( volume, 10 );
//
//            for ( List<VolumeDatapoint> vdp : dp.values() ) {
//                store.save( vdp );
//            }
//            List<VolumeDatapoint> results = store.findAll();
//            Assert.assertEquals( dp.size(), results.size() );
//        }
//    }

//    // Tests not implemented.
//    public static class JDOMetricRepositoryTest {
//        MetricRepository store = null;
//        @Before
//        public void setUp() {
//            store = new JDOMetricsRepository( DB_PATH );
//            final JSONArray array = new JSONArray( JSON );
//            for ( int i = 0;
//                  i < array.length();
//                  i++ ) {
//                JSON_DATAPOINTS.add( ObjectModelHelper.toObject( array.getJSONObject( i )
//                                                                      .toString(),
//                                                                 VolumeDatapoint.class ) );
//            }
//        }
//
//        public void create() {
//            store.save( new VolumeDatapointBuilder().withKey( Metrics.PUTS.key() )
//                                                    .withVolumeName( "TestVolume" )
//                                                    .withTimestamp( System.currentTimeMillis() / 1000 )
//                                                    .withValue( 1.0 )
//                                                    .build() );
//
//            Assert.assertTrue( Files.exists( Paths.get( DB_PATH ) ) );
//        }
//
//        // ignored.  findAll is failing b/c IVolumeDatapoint is not an entity. @Test
//        public void populate() {
//            JSON_DATAPOINTS.forEach( store::save );
//
//            List<VolumeDatapoint> results = store.findAll();
//            Assert.assertEquals( JSON_DATAPOINTS.size(), results.size() );
//        }
//
//        public void counts() {
//            System.out.println( "Total row count: " +
//                                store.countAll() );
//
//            System.out.println( "TestVolume row count: " +
//                                store.countAllBy(
//                                                    new VolumeDatapointBuilder()
//                                                        .withVolumeName( "TestVolume" )
//                                                             //.withKey( Metrics.PUTS.key() )
//                                                        .build() ) );
//        }
//
//        // ignored... findAll is failing b/c the IVolumeDatapoint is not an entity.  @Test
//        public void findAll() {
//            store.findAll()
//                 .forEach( System.out::println );
//        }
//
//        public void remove() {
//            System.out.println( "before: " + store.countAll() );
//            store.findAll()
//                 .stream()
//                 .filter( vdp -> vdp.getKey()
//                                    .equals( Metrics.LBYTES.key() ) )
//                 .forEach( store::delete );
//            System.out.println( "after: " + store.countAll() );
//        }
//
//        public void removeAll() {
//            System.out.println( "before: " + store.countAll() );
//            store.findAll()
//                 .forEach( store::delete );
//            System.out.println( "after: " + store.countAll() );
//        }
//
//        @After
//        public void cleanup() {
//            try {
//                if ( Files.exists( Paths.get( DB_PATH ) ) ) {
//                    Files.delete( Paths.get( DB_PATH ) );
//                }
//
//                if ( Files.exists( Paths.get( DB_PATH_TRANS_LOG ) ) ) {
//                    Files.delete( Paths.get( DB_PATH_TRANS_LOG ) );
//                }
//
//                if ( Files.exists( Paths.get( DB_PATH )
//                                        .getParent() ) ) {
//                    Files.delete( Paths.get( DB_PATH )
//                                       .getParent() );
//                }
//            } catch ( IOException e ) {
//                Assert.fail( e.getMessage() );
//            }
//        }
//    }
//
//    /**
//     * Since there is a directive inplace that states we shou
//     */
//    //  @Test
//    //  public void test()
//    //  {
//    //    setUp();
//    //    create();
//    //    populate();
//    //    counts();
//    //    findAll();
//    //    remove();
//    //    removeAll();
//    //    cleanup();
//    //  }
}
