/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen;

import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.Volume;
import com.formationds.tools.statgen.influxdb.InfluxDBStatWriter;
import com.google.common.collect.Lists;
import joptsimple.OptionParser;
import joptsimple.OptionSet;

import java.io.OutputStreamWriter;
import java.time.Duration;
import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Date;
import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.stream.Collectors;

/**
 * Generates a set of volume statistics over a given period of time and sends them to an OM for processing.
 */
// TODO: ultimately we would like for this tool to be usable to generate and send stats to the OM
// or InfluxDB directly, to the upcoming stats service, or just to a file.  As it stands right now
// we only send to the OM.  I have refactored OMConfigServiceRestClient into "VolumeManager" and
// "StatWriter" interfaces but haven't implemented influx, file, or other writers so right now it is OM only.
public class StatGenerator {

    public static final String ARG_WRITER        = "writer";
    public static final String ARG_OM_URL        = "om-url";
    public static final String ARG_USER          = "user";
    public static final String ARG_PWD           = "pwd";
    public static final String ARG_VOLUMES_COUNT = "volumes";
    public static final String ARG_VOL_PREFIX    = "prefix";
    public static final String ARG_SINCE         = "since";
    public static final String ARG_LAST          = "last";
    public static final String ARG_FREQ          = "freq";
    public static final String ARG_INFLUX_URL    = "influx";
    public static final String ARG_INFLUX_USER   = "influx_user";
    public static final String ARG_INFLUX_PWD    = "influx_pwd";
    public static final String ARG_INFLUX_DB     = "influx_db";

    private static boolean has( OptionSet opts, String a ) {
        return opts.has( a ) && opts.hasArgument( a );
    }

    private static <T> T get( OptionSet opts, String o, T dflt ) {
        if ( has( opts, o ) ) {
            T val = (T) opts.valueOf( o );
            if ( val != null && !val.toString().isEmpty() ) {
                return val;
            }
        }
        return dflt;
    }

    public static void main( String[] args ) throws Exception {

        OptionParser parser = new OptionParser();
        parser.allowsUnrecognizedOptions();
        parser.accepts( ARG_WRITER, "Stat Writer {OM|INFLUXDB}" ).withRequiredArg().ofType( String.class ).defaultsTo( "OM" );
        parser.accepts( ARG_OM_URL, "Specify the OM REST API url" ).withRequiredArg().ofType( String.class )
              .defaultsTo( "http://localhost:7777" );
        parser.accepts( ARG_INFLUX_URL, "Specify the InfluxDB REST API url (incl. db)" )
              .withRequiredArg()
              .ofType( String.class )
              .defaultsTo( "http://localhost:8086/" );

        parser.accepts( ARG_USER, "OM user name" ).withRequiredArg().ofType( String.class ).defaultsTo( "root" );
        parser.accepts( ARG_PWD, "OM password" ).withRequiredArg().ofType( String.class );

        parser.accepts( ARG_INFLUX_USER, "InfluxDB user name" ).withRequiredArg().ofType( String.class ).defaultsTo( "admin" );
        parser.accepts( ARG_INFLUX_PWD, "InfluxDB password" ).withRequiredArg().ofType( String.class );
        parser.accepts( ARG_INFLUX_DB, "InfluxDB database" ).withRequiredArg().ofType( String.class ).defaultsTo( "om-metricdb" );

        parser.accepts( ARG_VOLUMES_COUNT, "Number of volumes to generate." ).withRequiredArg().ofType( Integer.class )
              .defaultsTo( 10 );
        parser.accepts( ARG_VOL_PREFIX, "Volume name prefix" ).withRequiredArg().ofType( String.class )
              .defaultsTo( DFLT_VOL_PREFIX );
        parser.accepts( ARG_SINCE, "Start date for producing stats.  One of since or last may be specified.  " +
                                   "If --since is provided, it takes precedence over --last" ).withRequiredArg()
              .ofType( Date.class );
        parser.accepts( ARG_LAST, "Specifies production of stats over the last n hours." ).withRequiredArg()
              .ofType( Integer.class );
        parser.accepts( ARG_FREQ, "Specifies the frequency or period (in seconds) of the generated stats." )
              .withRequiredArg()
              .defaultsTo( "60" )
              .ofType( Integer.class );

        parser.acceptsAll( Lists.newArrayList( "h", "?", "help" ), "show help" ).forHelp();

        OptionSet options = parser.parse( args );

        if ( options.has( "h" ) || options.has( "help" ) || options.has( "?" ) ) {
            parser.printHelpOn( new OutputStreamWriter( System.out ) );
            return;
        }

        final String url = get( options, ARG_OM_URL, "http://localhost:7777" );

        final String influxUrl = get( options, ARG_INFLUX_URL, "http://localhost:8086" );
        final String influxUser = get( options, ARG_INFLUX_USER, "root" );
        final String influxPwd = get( options, ARG_INFLUX_PWD, "root" );
        final String influxDb = get( options, ARG_INFLUX_DB, "om-metricdb" );

        final String protocol = url.split( "://" )[0];
        final String[] hostport = url.split( "://" )[1].split( ":" );
        final String host = hostport[0];
        final int port = Integer.valueOf( hostport[1] );

        final String user = get( options, ARG_USER, "admin" );

        // todo: encrypt or prompt
        final String pwd = get( options, ARG_PWD, "admin" );

        final int volumes = get( options, ARG_VOLUMES_COUNT, 100 );
        final String volumePrefix = get( options, ARG_VOL_PREFIX, DFLT_VOL_PREFIX );

        final int freq = get( options, ARG_FREQ, 60 );

        LocalDateTime since = (has( options, ARG_SINCE ) ?
                               LocalDateTime.parse( get( options, ARG_SINCE, null ) ) :
                               null);

        if ( since == null ) {
            since = LocalDateTime.now().minus( get( options, ARG_LAST, 24 ), ChronoUnit.HOURS );
        }

        // TODO: support file, Influx, and other potential writers.
        OMConfigServiceRestClient client = new OMConfigServiceRestClient( protocol, host, port );
        client.login( user, pwd );

        StatWriter statWriter = null;
        final String writerOpt = get( options, ARG_WRITER, "OM" );
        if (writerOpt.equalsIgnoreCase( "OM" )) {
            statWriter = client;
        } else if (writerOpt.equalsIgnoreCase( "INFLUXDB" )) {
            statWriter = new InfluxDBStatWriter(influxUrl, influxUser, influxPwd, influxDb);
        }
        StatGenerator gen = new StatGenerator( client,
                                               statWriter,
                                               since,
                                               Duration.of( freq, ChronoUnit.SECONDS ),
                                               volumes,
                                               volumePrefix );
        gen.sendStats();
    }

    // does not include firebreak metrics... using separate FirebreakGenerator for that.
    @SuppressWarnings("unchecked")
    protected static final EnumMap<Metrics, StatValueGenerator<Number>> DFLT_GENERATORS =
        new EnumMap( Metrics.class ) {{
            put( Metrics.PUTS, StatValueGenerators.sequentialInt( 1 ) );
            put( Metrics.GETS, StatValueGenerators.sequentialInt( 100 ) );
            put( Metrics.QFULL, StatValueGenerators.constant( 0.0D ) );
            put( Metrics.SSD_GETS, StatValueGenerators.constant( 100L ) );
            put( Metrics.LBYTES, StatValueGenerators.sequentialLong( 100L ) );
            put( Metrics.PBYTES, StatValueGenerators.sequentialLong( 0L ) );
            put( Metrics.MBYTES, StatValueGenerators.sequentialLong( 0L ) );
            put( Metrics.UBYTES, StatValueGenerators.sequentialLong( 0L ) );
            put( Metrics.BLOBS, StatValueGenerators.sequentialLong( 0L ) );
            put( Metrics.OBJECTS, StatValueGenerators.sequentialLong( 0L ) );
            put( Metrics.ABS, StatValueGenerators.addDouble( 0.0D, 0.1F ) );
            put( Metrics.AOPB, StatValueGenerators.addDouble( 0.0D, 0.1F ) );
        }};

    static final FirebreakGenerator DFLT_FB_GENERATOR = new FirebreakGenerator();

    public static final String DFLT_VOL_PREFIX = "test_vol_";

    /** */
    VolumeManager volumeManager;

    /** */
    StatWriter writer;

    /** time range for generation of stats */
    LocalDateTime startTime;

    /**
     * end time for the stats generation.
     */
    LocalDateTime endTime;

    /** the frequency to simulate the stats */
    Duration frequency;

    /** */
    EnumMap<Metrics, StatValueGenerator<Number>> metricValueGenerators;

    /** firebreak stats are (currently?) handled separately from other metrics */
    FirebreakGenerator firebreakGenerator;

    /** number of volumes to create and simulate stats for */
    int volumeCnt;

    /** prefix for generated volumes.  volume type and a sequence number are appended. */
    String volumePrefix = DFLT_VOL_PREFIX;

    private transient Map<String, Volume> volumeCache;

    public StatGenerator( VolumeManager volumeManager,
                          StatWriter writer,
                          int volumeCnt ) {
        this( volumeManager,
              writer,
              LocalDateTime.now().minus( 1, ChronoUnit.DAYS ),
              LocalDateTime.now(),
              Duration.of( 1, ChronoUnit.MINUTES ),
              volumeCnt, DFLT_VOL_PREFIX, DFLT_GENERATORS,
              DFLT_FB_GENERATOR
        );
    }

    public StatGenerator( VolumeManager volumeManager,
                          StatWriter writer,
                          LocalDateTime startTime,
                          Duration frequency,
                          int volumeCnt,
                          String volumePrefix ) {
        this( volumeManager,
              writer,
              startTime,
              LocalDateTime.now(),
              frequency,
              volumeCnt,
              volumePrefix,
              DFLT_GENERATORS,
              DFLT_FB_GENERATOR );
    }

    public StatGenerator( VolumeManager volumeManager,
                          StatWriter writer,
                          LocalDateTime startTime,
                          LocalDateTime endTime,
                          Duration frequency,
                          int volumes,
                          String volumePrefix,
                          EnumMap<Metrics, StatValueGenerator<Number>> metricValueGenerators,
                          FirebreakGenerator firebreakGenerator ) {
        this.volumeManager = volumeManager;
        this.writer = writer;
        this.startTime = startTime;
        this.endTime = endTime;
        this.frequency = frequency;
        this.metricValueGenerators = metricValueGenerators;
        this.firebreakGenerator = firebreakGenerator;
        this.volumeCnt = volumes;
        this.volumePrefix = volumePrefix;
    }

    protected void loadVolumeCache() {
        final List<Volume> volumes = volumeManager.listVolumes( "" );
        if ( volumeCache != null ) { volumeCache.clear(); }

        volumeCache = new TreeMap<>( volumes.stream()
                                            .collect( Collectors.toMap( Volume::getName,
                                                                        volume -> volume ) ) );
    }

    /**
     * Check if volumes exist and if not create them.
     */
    public void init() {
        loadVolumeCache();

        for ( int i = 0; i < volumeCnt; i++ ) {
            String vname = volumePrefix + i;
            if ( !volumeCache.containsKey( vname ) ) {
                if ( i % 2 == 0 ) {
                    volumeManager.createBlockVolume( "statgen", vname, Tenant.SYSTEM, Size.gb( 100 ), Size.mb( 2 ) );
                } else {
                    volumeManager.createObjectVolume( "statgen", vname, Tenant.SYSTEM, Size.gb( 100 ) );
                }
            }
        }

        // reload the cache
        loadVolumeCache();
    }

    public void sendStats() {
        init();

        LocalDateTime time = startTime;
        while ( time.isBefore( endTime ) ) {

            Map<Volume, List<VolumeDatapoint>> vdps = generateStats( time );

            List<VolumeDatapoint> collapsed = new ArrayList<>();
            vdps.values().stream().forEach( collapsed::addAll );
            writer.sendStats( collapsed );

            time = time.plus( frequency );
        }
    }

    protected Map<Volume, List<VolumeDatapoint>> generateStats( LocalDateTime time ) {
        Map<Volume, List<VolumeDatapoint>> stats = new LinkedHashMap<>();

        // todo: using same fb info for each volume.  this means that every volume will show
        // a firebreak at the same time...
        EnumMap<FirebreakType, FirebreakMetrics> fb = firebreakGenerator.generate();

        volumeCache.entrySet().stream().forEach( ve -> {
            Volume volume = ve.getValue();
            List<VolumeDatapoint> vstats = new ArrayList<>();
            metricValueGenerators.entrySet().stream().forEach( me -> {
                VolumeDatapoint vdp = new VolumeDatapoint( time.toEpochSecond( ZoneOffset.UTC ),
                                                           volume.getId(),
                                                           volume.getName(),
                                                           me.getKey().name(),
                                                           me.getValue().generate().doubleValue() );
                vstats.add( vdp );
            } );

            fb.entrySet().stream().forEach( fbe -> {
                FirebreakType type = fbe.getKey();
                FirebreakMetrics fbm = fbe.getValue();

                for ( Metrics m : Metrics.FIREBREAK ) {
                    VolumeDatapoint vdp = new VolumeDatapoint( time.toEpochSecond( ZoneOffset.UTC ),
                                                               volume.getId(),
                                                               volume.getName(),
                                                               m.name(),
                                                               fbm.get( m ) );
                    vstats.add( vdp );
                }
            } );
            stats.put( volume, vstats );
        } );

        return stats;
    }
}
