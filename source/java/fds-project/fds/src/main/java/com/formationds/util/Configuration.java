/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.util;

import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.nfs.XdiStaticConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.sun.management.HotSpotDiagnosticMXBean;
import com.sun.management.VMOption;
import joptsimple.OptionParser;
import joptsimple.OptionSet;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.io.File;
import java.io.IOException;
import java.lang.management.ManagementFactory;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.ParseException;
import java.util.List;
import java.util.Optional;

public class Configuration {
    public static final String KEYSTORE_PATH = "fds.ssl.keystore_path";
    public static final String KEYSTORE_PASSWORD = "fds.ssl.keystore_password";
    public static final String KEYMANAGER_PASSWORD = "fds.ssl.keymanager_password";
    public static final String FDS_ROOT_SYS_PROP = "fds-root";
    public static final String FDS_ROOT_ENV = "fds_root";

    public static final String FDS_XDI_NFS_THREAD_POOL_SIZE                 = "fds.xdi.nfs_thread_pool_size";
    public static final String FDS_XDI_NFS_STATS = "fds.xdi.nfs_stats";
    public static final String FDS_XDI_NFS_DEFER_METADATA_UPDATES = "fds.xdi.nfs_defer_metatada_updates";
    public static final String FDS_XDI_NFS_MAX_LIVE_NFS_COOKIES = "fds.xdi.nfs_max_live_nfs_cookies";
    public static final String FDS_XDI_AM_TIMEOUT_SECONDS = "fds.xdi.am_timeout_seconds";
    public static final String FDS_XDI_AM_RETRY_ATTEMPTS = "fds.xdi.am_retry_attempts";
    public static final String FDS_XDI_AM_RETRY_INTERVAL_SECONDS = "fds.xdi.am_retry_interval_seconds";
    public static final String FDS_OM_IP_LIST = "fds.common.om_ip_list";
    public static final String FDS_OM_UUID = "fds.common.om_uuid";
    public static final String FDS_OM_NODE_UUID = "fds.pm.platform.uuid";

    // Stats tunable
    public static final String FDS_OM_STATS_FREQUENCY = "fds.om.stats.frequency";
    public static final String FDS_OM_STATS_DURATION = "fds.om.stats.duration";
    public static final String FDS_OM_STATS_METHOD = "fds.om.stats.method";

    private final String commandName;
    private final File   fdsRoot;

    /**
     * Create a new configuration for the named command with its arguments.  Supported
     * arguments processed by this class include
     * <ul>
     *     <li>fds-root</li>
     *     <li>console</li>
     * </ul>
     * Any other arguments are ignored.
     * </p>
     * If the system property "fds.service.name" is not set, it will be set based on the
     * command name.  The fds_root and fds.service.name system property are used for configuration of the
     * logging system.
     * </p>
     * If the "fds-root" command-line argument is not specified, the FDS root is determined
     * first based on the presence of the "fds_root" environment variable, followed by the
     * "fds-root" system property, and finally by the "fds_root" system property (Yes, we need
     * to make this more consistent!)
     *
     * @param commandName the name of the command for the log name if the fds.service.name property is not set.
     * @param commandLineArgs
     */
    public Configuration( String commandName, String... commandLineArgs ) {
        this.commandName = commandName;

        // if fds.service.name property is not set, update it with the command name.
        // this is intended to happen before any logging configurations are initialized, but
        // when the -DconfigurationFile is used, it is possible this is already initialized
        // before we get here.
        if ( System.getProperty( "fds.service.name" ) == null ) {
            System.setProperty( "fds.service.name", commandName );
        }

        OptionParser parser = new OptionParser();
        parser.allowsUnrecognizedOptions();
        parser.accepts("fds-root").withRequiredArg();
        parser.accepts("console");
        OptionSet options = parser.parse(commandLineArgs);

        fdsRoot = resolveFdsRoot( options );

        // set the fds-root system property based on the resolved root
        System.setProperty( FDS_ROOT_SYS_PROP, fdsRoot.getAbsolutePath() );

        initDiagnostics(fdsRoot);

        initJaas(fdsRoot);
    }

    private File resolveFdsRoot( OptionSet options ) {
        File root = null;
        if (options.has("fds-root")) {
            root = new File((String) options.valueOf("fds-root"));

            // todo: validate that fdsRoot matches the env setting for "fds_root".  The logging config
            // depends on the setting of fds_root.
        } else {

            String envFdsRoot = System.getenv( FDS_ROOT_ENV );
            if ( envFdsRoot == null || envFdsRoot.isEmpty() ) {
                String sysFdsRoot = System.getProperty( FDS_ROOT_SYS_PROP, System.getProperty( FDS_ROOT_ENV, "/fds" ) );
                root = new File( sysFdsRoot );
            } else {
                root = new File( envFdsRoot );
            }
        }
        return root;
    }

    /**
     * Load the system core pattern path based on /proc/sys/kernel/core_pattern
     *
     * @return the directory specified by the core pattern, or Optional.empty() if a directory is not specified.
     */
    private Optional<Path> loadSystemCorePatternPath(  ) {
        try {
            Path corePatternFile = Paths.get( "/proc/sys/kernel", "core_pattern" );
            List<String> lines = Files.readAllLines( corePatternFile );
            if ( lines.size() < 1 )
                return Optional.empty();

            if ( lines.size() > 1 ) {
                throw new IllegalStateException( "Unexpected lines returned from sysctl command.  Expecting 1 line but got: " +
                                                 lines );
            }

            String cp = lines.get( 0 );

            Path cfpattern = Paths.get( cp );
            if (cfpattern == null)
                return Optional.empty();

            if (Files.isDirectory( cfpattern )) {
                return Optional.of( cfpattern );
            }

            // traverse the core pattern directories (if any) to identify
            Path p = cfpattern;

            //noinspection StatementWithEmptyBody
            while ( !Files.isDirectory( p ) && (p = p.getParent()) != null );

            Path root = cfpattern.getRoot();
            if ( root == null || p == null || root.equals( p ) ) {
                return Optional.empty();
            }

            return Optional.of( p );

        } catch (IOException ioe) {
            // failed to read... most likely permissions, maybe not an ubuntu linux system with expected path
            Logger logger = LogManager.getLogger( Configuration.class );
            logger.warn( "Failed to determine kernel core pattern directory via sysctl command: " +
                         ioe.getMessage() );

            return Optional.empty();
        }
    }

    /**
     * Initialize the JVM diagnostics for HeapDumpOnOutOfMemoryError and HeapDumpPath.
     * </p>
     * If a HeapDumpPath is specified in the VMOptions from the Java command-line, and it is a valid writable
     * directory, it is used.  Otherwise, we first attempt to load the system kernel.core_pattern setting via the
     * sysctl tool via #loadSystemCorePatternPath.  If that does not resolve to a valid writable directory, then we
     * attempt to default to /corefiles.  If that directory does not exist or is not writable, then we default to the
     * fds bin directory.
     * </p>
     * Note that we also want to modify the -XX:ErrorFile setting here but as of Java 8 it is not manageable
     * at runtime and must be set on the command-line in order to override the default location which is the CWD of the process..
     *
     * @param fdsRoot
     */
    private void initDiagnostics(File fdsRoot)
    {
        Logger logger = LogManager.getLogger( Configuration.class );
        if ( ManagementFactory.getRuntimeMXBean().getVmVendor().toLowerCase().contains( "oracle" ) ) {

            Path dfltCorefilesDir = Paths.get( fdsRoot.getAbsolutePath() + "/var/log/corefiles" );
            Optional<Path> systemCorePatternLocation = loadSystemCorePatternPath();

            if ( !systemCorePatternLocation.isPresent() ) {
                logger.debug( "Using default corefiles location: {}", dfltCorefilesDir );
            }
            Path coreFilesDir = systemCorePatternLocation.orElse( dfltCorefilesDir );
            Path binDir = Paths.get( fdsRoot.getAbsolutePath(), "bin" );

            try {
                HotSpotDiagnosticMXBean hotspot = ManagementFactory.getPlatformMXBean( HotSpotDiagnosticMXBean.class );
                VMOption heapDumpPathOption = hotspot.getVMOption( "HeapDumpPath" );
                Path heapDumpPath = null;
                boolean useExistingPath = false;
                if ( heapDumpPathOption != null &&
                     heapDumpPathOption.getValue() != null && !heapDumpPathOption.getValue().isEmpty() )  {

                     heapDumpPath = Paths.get( heapDumpPathOption.getValue() );

                }

                Path hprofDir;
                if ( heapDumpPath != null &&
                     Files.exists( heapDumpPath ) &&
                     Files.isDirectory( heapDumpPath ) &&
                     Files.isWritable( heapDumpPath ) ) {

                    hprofDir = heapDumpPath;
                    useExistingPath = true;

                } else if ( Files.exists( coreFilesDir ) &&
                            Files.isDirectory( coreFilesDir ) &&
                            Files.isWritable( coreFilesDir ) ) {

                    hprofDir = coreFilesDir;

                } else {

                    hprofDir = binDir;

                }

                logger.info( "Enabling JVM OOME diagnostics (hprof) with {}HeapDumpPath output directory: {}",
                             (useExistingPath ? "command-line settings for " : ""),
                             hprofDir );

                String vmName = ManagementFactory.getRuntimeMXBean().getName();
                String pidS = vmName.split( "@" )[0];
                hotspot.setVMOption( "HeapDumpOnOutOfMemoryError", "true" );

                if ( !useExistingPath ) {
                    hotspot.setVMOption( "HeapDumpPath",
                                         String.format( "%s/%s_pid%s.hprof",
                                                        hprofDir,
                                                        commandName,
                                                        pidS ) );
                }
            } catch (Exception e) {
                logger.warn( "Failed to set the JVM diagnostic options.  Running with default diagnostics", e );
            }
        } else {
            logger.warn( "Unexpected JVM Vendor name '{}'.  Oracle JVM is expected in order to init JVM diagnostics." );
        }
    }

    private void initJaas(File fdsRoot) {
        try {

            Path jaasConfig = Paths.get( fdsRoot.getCanonicalPath(),
                                         "etc",
                                         "auth.conf" );
            URL configUrl = jaasConfig.toFile().toURI().toURL();
            System.setProperty( "java.security.auth.login.config",
                                configUrl.toExternalForm() );

        } catch( IOException e ) {

            throw new RuntimeException(e);

        }
    }

    public String getFdsRoot() {
        return fdsRoot.getAbsolutePath();
    }

    public Path getPlatformConfigPath() {
        return Paths.get( getFdsRoot(), "etc", "platform.conf" );
    }

    public ParsedConfig getPlatformConfig() {
        Path path = getPlatformConfigPath();
        return getParserFacade( path );

    }


    /**
     * @return Returns the demo configuration
     * @deprecated Will be removed very soon.
     */
    @Deprecated
    public ParsedConfig getDemoConfig() {

        Path path = Paths.get( getFdsRoot(), "etc", "demo.conf" );
        return getParserFacade(path);

    }

    private ParsedConfig getParserFacade( final Path path ) {
        try {

            return new ParsedConfig(Files.newInputStream(path));

        } catch( ParseException | IOException e ) {

            throw new RuntimeException( e );

        }
    }

    /**
     * The OM currently only supports one address but is intended to eventually support multiples.
     *
     * @return the OM IP addresses
     *
     * @throws IllegalStateException if the fds.common.om_ip_list configuration contains more than one address
     */
    public String getOMIPAddress() {
        ParsedConfig platformConfig = getPlatformConfig();
        String omHost = platformConfig.defaultString( FDS_OM_IP_LIST, "localhost" );
        String[] hosts = omHost.contains( "," ) ? omHost.split( "," ) : new String[] {omHost};

        if ( hosts.length > 1 ) {
            throw new IllegalStateException( "Unsupported OM IP List (" + FDS_OM_IP_LIST + ") " +
                                             "configuration. Multi-OM not currently supported" );
        }

        return hosts[0];
    }

    public long getOMUuid() {
        ParsedConfig platformConfig = getPlatformConfig();
        final int omUuid = platformConfig.defaultInt( FDS_OM_UUID, 1028 );

        return ( long ) omUuid;
    }

    public long getOMNodeUuid() {
        ParsedConfig platformConfig = getPlatformConfig();
        final int omNodeUuid = platformConfig.defaultInt( FDS_OM_NODE_UUID, 1024 );

        return ( long ) omNodeUuid;
    }

    public long getOmStatsDuration()
    {
        return ( long ) getPlatformConfig().defaultInt( FDS_OM_STATS_DURATION, -1 );
    }

    public long getOmStatsFrequency()
    {
        return ( long ) getPlatformConfig().defaultInt( FDS_OM_STATS_FREQUENCY, 120 );
    }

    public String getOmStatsMethod()
    {
        return getPlatformConfig().defaultString( FDS_OM_STATS_METHOD, HttpMethod.POST.name() )
                                  .toUpperCase();
    }

    public XdiStaticConfiguration getXdiStaticConfig( int pmPort ) {
        ParsedConfig platformConfig = getPlatformConfig();
        return new XdiStaticConfiguration(
                platformConfig.lookup(FDS_XDI_NFS_THREAD_POOL_SIZE).intValue(),
                platformConfig.lookup(FDS_XDI_NFS_STATS).booleanValue(),
                platformConfig.lookup(FDS_XDI_NFS_DEFER_METADATA_UPDATES).booleanValue(),
                platformConfig.lookup(FDS_XDI_NFS_MAX_LIVE_NFS_COOKIES).intValue(),
                platformConfig.lookup(FDS_XDI_AM_TIMEOUT_SECONDS).intValue(),
                platformConfig.lookup(FDS_XDI_AM_RETRY_ATTEMPTS).intValue(),
                platformConfig.lookup(FDS_XDI_AM_RETRY_INTERVAL_SECONDS).intValue(),
                // assign NFS statistics webapp port based on platform port number 7000 - 1445 == 5555
                pmPort - 1445 );
    }
}
