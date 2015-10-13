/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.util;

import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.nfs.NfsConfiguration;
import com.sun.management.HotSpotDiagnosticMXBean;
import com.sun.management.VMOption;
import joptsimple.OptionParser;
import joptsimple.OptionSet;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.core.Appender;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.appender.ConsoleAppender;
import org.apache.logging.log4j.core.appender.RollingRandomAccessFileAppender;
import org.apache.logging.log4j.core.appender.rolling.*;
import org.apache.logging.log4j.core.config.AppenderRef;
import org.apache.logging.log4j.core.config.LoggerConfig;
import org.apache.logging.log4j.core.config.Property;
import org.apache.logging.log4j.core.layout.PatternLayout;
import org.apache.logging.log4j.status.StatusLogger;

import java.io.File;
import java.io.IOException;
import java.lang.management.ManagementFactory;
import java.net.URI;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.ParseException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public class Configuration {
    public static final String KEYSTORE_PATH = "fds.ssl.keystore_path";
    public static final String KEYSTORE_PASSWORD = "fds.ssl.keystore_password";
    public static final String KEYMANAGER_PASSWORD = "fds.ssl.keymanager_password";
    public static final String TIME_STAMP_FORMAT = "yyyy-MM-dd'T'HH:mm:ss.SSSSSSZZZ";
    public static final String FDS_ROOT_SYS_PROP = "fds-root";
    public static final String FDS_ROOT_ENV = "fds_root";

    //trace/debug/normal/info/notify/notification/crit/critical,warn/warning/error
    private static final Map<String, String> LOGLEVELS = new HashMap<>();
    static {
        LOGLEVELS.put("trace", "TRACE");
        LOGLEVELS.put("debug", "DEBUG");
        LOGLEVELS.put("normal", "INFO");
        LOGLEVELS.put("notification", "INFO");
        LOGLEVELS.put("warning", "WARN");
        LOGLEVELS.put("error", "ERROR");
        LOGLEVELS.put("critical", "FATAL");
    }

    public static final String FDS_XDI_NFS_THREAD_POOL_SIZE                 = "fds.xdi.nfs_thread_pool_size";
    public static final String FDS_XDI_NFS_THREAD_POOL_QUEUE_SIZE           = "fds.xdi.nfs_thread_pool_queue_size";
    public static final String FDS_XDI_NFS_INCOMING_REQUEST_TIMEOUT_SECONDS = "fds.xdi.nfs_incoming_request_timeout_seconds";
    public static final String FDS_XDI_NFS_STATS = "fds.xdi.nfs_stats";
    public static final String FDS_XDI_NFS_DEFER_METADATA_UPDATES = "fds.xdi.nfs_defer_metatada_updates";
    public static final String FDS_XDI_NFS_MAX_LIVE_NFS_COOKIES = "fds.xdi.nfs_max_live_nfs_cookies";

    public static final String FDS_OM_IP_LIST = "fds.common.om_ip_list";
    public static final String FDS_OM_UUID = "fds.common.om_uuid";
    public static final String FDS_OM_NODE_UUID = "fds.pm.platform.uuid";

    private final String commandName;
    private final File   fdsRoot;
    private FdsLogConfig logConfig;

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

        String logLevel = getPlatformConfig().defaultString("fds.pm.log_severity", "normal").toLowerCase();

        logConfig = new FdsLogConfig();
        if (options.has("console")) {
            logConfig.initConsoleLogging("DEBUG");
        } else {
            logConfig.initFileLogging(commandName, LOGLEVELS.getOrDefault(logLevel, "INFO"));
        }

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

            Path dfltCorefilesDir = Paths.get( "/corefiles" );
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

    // TODO: refactor this into something more sensible and address fs-2538
    // programmatic config of log4j2 is proving to be a pain to get just right and seems to be
    // somewhat dependent on whether or not some jar in the classpath happens to have a log4j2
    // config file in its resources. It might be more reasonable to put a default config in the
    // main jar resources (though that might still be dependent on the order in which they are
    // searched, if multiple jars contain log configs)
    public class FdsLogConfig {

        private void initConsoleLogging( String loglevel ) {
            final LoggerContext ctx = (LoggerContext) LogManager.getContext( false );
            final org.apache.logging.log4j.core.config.Configuration config = ctx.getConfiguration();
            LoggerConfig fdsRootConfig = config.getLoggerConfig( "com.formationds" );
            Level level = Level.getLevel( loglevel );
            if ( level == null ) level = Level.FATAL;

            Appender consoleAppender = fdsRootConfig.getAppenders().get( "console" );
            if ( consoleAppender == null ) {
                consoleAppender = getConsoleAppender();
                fdsRootConfig.addAppender( consoleAppender, level, null );
                fdsRootConfig.setAdditive( false );
            }

            fdsRootConfig.setLevel( level );

            if (!config.getLoggers().containsKey( "com.formationds" ) ) {
                config.addLogger( "com.formationds", fdsRootConfig );
            }

            ctx.updateLoggers( config );
        }

        /**
         * Get the default console appender, creating it and adding to the configuration if necessary.
         *
         * @return the console appender, referenceable via the name "console"
         */
        private ConsoleAppender getConsoleAppender() {
            final LoggerContext ctx = (LoggerContext) LogManager.getContext( false );
            final org.apache.logging.log4j.core.config.Configuration config = ctx.getConfiguration();

            Appender consoleAppender = config.getAppender( "console" );
            if ( consoleAppender == null || !consoleAppender.getClass().isInstance( ConsoleAppender.class ) ) {
                PatternLayout layout = PatternLayout.newBuilder()
                                                    .withAlwaysWriteExceptions( true )
                                                    .withPattern( "%d{" + TIME_STAMP_FORMAT + "} - %-5p %c %x - %m%n" )
                                                    .build();
                consoleAppender = ConsoleAppender.newBuilder().setName( "console" ).setLayout( layout ).build();
                consoleAppender.start();

                config.addAppender( consoleAppender );
            }
            return (ConsoleAppender) consoleAppender;
        }

        private Appender getServiceAppender() {
            final LoggerContext ctx = (LoggerContext) LogManager.getContext( false );
            org.apache.logging.log4j.core.config.Configuration config = ctx.getConfiguration();

            final String serviceAppenderName = String.format( "fds-%s", commandName );
            Appender serviceAppender = config.getAppender( serviceAppenderName );

            if ( serviceAppender == null ) {
                PatternLayout layout = PatternLayout.newBuilder()
                                                    .withAlwaysWriteExceptions( true )
                                                    .withPattern( "%d{" + TIME_STAMP_FORMAT + "} - %-5p %c %x - %m%n" )
                                                    .build();

                final Path fdsLogDir = getFdsLogDir();
                String fileName = fdsLogDir.resolve( commandName + ".log" ).toString();
                String filePattern = fdsLogDir + "/archive/" + commandName + "-%d{yyyy-MM-dd}-%i.log.gz";
                String append = Boolean.toString( true );
                String immediateFlush = Boolean.toString( true );
                String bufferSizeStr = null;
                TriggeringPolicy policy = CompositeTriggeringPolicy.createPolicy( TimeBasedTriggeringPolicy
                                                                                      .createPolicy( "1", Boolean
                                                                                                              .toString( true ) ),
                                                                                  SizeBasedTriggeringPolicy
                                                                                      .createPolicy( "100 MB" ) );
                RolloverStrategy strategy = DefaultRolloverStrategy.createStrategy( "100", "1", "max", "5", config );
                Filter filter = null;
                String ignore = null;
                String advertise = null;
                String advertiseURI = null;

                serviceAppender = RollingRandomAccessFileAppender.createAppender( fileName,
                                                                                  filePattern,
                                                                                  append,
                                                                                  serviceAppenderName,
                                                                                  immediateFlush,
                                                                                  bufferSizeStr,
                                                                                  policy,
                                                                                  strategy,
                                                                                  layout,
                                                                                  filter,
                                                                                  ignore,
                                                                                  advertise,
                                                                                  advertiseURI,
                                                                                  config );

                serviceAppender.start();
                config.addAppender( serviceAppender );
            }

            return serviceAppender;
        }

        /**
         * Initialize file logging.
         *
         * @param commandName
         * @param loglevel
         */
        private void initFileLogging( String commandName, String loglevel ) {
            final LoggerContext ctx = (LoggerContext) LogManager.getContext( false );

            // determine if logging was configured via an external configuration.
            // This depends on the fact that this configuration happens early in the startup process
            // before any alternative configuration is loaded.
            URI configLocation = ctx.getConfigLocation();
            if ( configLocation != null ) {
                return;
            }

            // we still want to override the log level on the com.formationds logger.
            Level level = Level.getLevel( loglevel );
            if ( level == null ) {
                throw new IllegalArgumentException( "Invalid log level for " +
                                                    commandName +
                                                    " specified in platform.conf" );
            }

            org.apache.logging.log4j.core.config.Configuration config = ctx.getConfiguration();

            LoggerConfig fdsRootConfig = null;
            Path dfltLog4jCfg = getFdsConfigFile( "log4j2.xml" );
            if ( Files.isReadable( dfltLog4jCfg ) ) {

                StatusLogger.getLogger().log( Level.WARN,
                                              "Log4j2 Configuration File not specified or not found.  " +
                                              "Attempting to load default configuration file {}.",
                                              dfltLog4jCfg);

                // set the log4j configuration (which internally triggers a reconfiguration)
                ctx.setConfigLocation( dfltLog4jCfg.toUri() );

                // reload the configuration and then set the level based on platform.conf
                config = ctx.getConfiguration();
                fdsRootConfig = getFdsRootLogConfig( config );
                fdsRootConfig.setLevel( level );

            } else {

                // at this point we have determined that the JVM was not started with a logging configuration
                // and that the default log4j2.xml file in the $fdsRoot/etc directory is not readable,
                // so we are going to programmatically configure a default logger as a last resort.

                StatusLogger.getLogger().log( Level.WARN,
                                              "Log4j2 Configuration File not specified and " +
                                              "default {} not found.  Configuring Log4j " +
                                              "with defaults.",
                                              dfltLog4jCfg );

                // configure the root logger to get only fatal messages, only if not already configured
                LoggerConfig rootConfig = config.getLoggerConfig( LogManager.ROOT_LOGGER_NAME );
                rootConfig.addAppender( getConsoleAppender(), Level.FATAL, null );
                rootConfig.setLevel( Level.FATAL );

                // all other logging should be captured under the fds root configuration
                // this will get an existing config if available, or create a new one
                fdsRootConfig = getFdsRootLogConfig( config );
                fdsRootConfig.setLevel( level );

                final String serviceAppenderName = String.format( "fds-%s", commandName );
                Appender serviceAppender = fdsRootConfig.getAppenders().get( serviceAppenderName );

                if ( serviceAppender == null ) {
                    serviceAppender = getServiceAppender();
                    fdsRootConfig.addAppender( serviceAppender, level, null );
                    fdsRootConfig.start();
                }

            }

            if (!config.getLoggers().containsKey( "com.formationds" ) ) {
                config.addLogger( "com.formationds", fdsRootConfig );
            }

            // lastly, update the loggers through the context
            ctx.updateLoggers(config);

        }

        /**
         * Get the FDS Root log config (com.formationds) creating it if necessary.
         *
         * @param config the log configuration
         *
         * @return the FDS root log config
         */
        private LoggerConfig getFdsRootLogConfig( org.apache.logging.log4j.core.config.Configuration config) {
            return getOrCreateLogConfig( "com.formationds", false, config );
        }

        /**
         * Get the log configuration for the logger with the specified name, creating it if logger is
         * not additive (does not inherit from parent loggers).
         * </p>
         * If the logger configuration is created, it does not have any appenders, filters, or properties defined and
         * the default level is Level#ERROR.
         *
         * @param name the name of the logger to find configuration for
         * @param additive whether or not the logger is additive (inherits parent configuration)
         * @param config the global log configuration
         *
         * @return the log configuration for the named logger
         */
        private LoggerConfig getOrCreateLogConfig( String name,
                                                   boolean additive,
                                                   org.apache.logging.log4j.core.config.Configuration config ) {
            LoggerConfig rootConfig = config.getLoggerConfig( LogManager.ROOT_LOGGER_NAME );
            LoggerConfig fdsRootConfig = config.getLoggerConfig( name );
            if ( ! (additive && rootConfig.getName().equals( name )) ) {
                try {
                    fdsRootConfig =
                        LoggerConfig
                            .createLogger( Boolean.toString( additive ), Level.ERROR, name, null, new AppenderRef[0], new Property[0], config,
                                           null );
                } catch (Exception e) {
                    e.printStackTrace();
                    throw e;
                }
            }
            return fdsRootConfig;
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

    public Path getFdsConfigDir() {
        return Paths.get( fdsRoot.getAbsolutePath(), "etc" );
    }

    public Path getFdsConfigFile(String fileName) {
        return getFdsConfigDir().resolve( fileName );
    }

    public String getFdsRoot() {
        return fdsRoot.getAbsolutePath();
    }

    public Path getFdsRootPath() {
        return fdsRoot.getAbsoluteFile().toPath();
    }

    public Path getFdsLogDir() {
        return getFdsRootPath().resolve( "var/logs" );
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

    public NfsConfiguration getNfsConfig() {
        ParsedConfig platformConfig = getPlatformConfig();
        return new NfsConfiguration(
                platformConfig.lookup(FDS_XDI_NFS_THREAD_POOL_SIZE).intValue(),
                platformConfig.lookup(FDS_XDI_NFS_THREAD_POOL_QUEUE_SIZE).intValue(),
                platformConfig.lookup(FDS_XDI_NFS_INCOMING_REQUEST_TIMEOUT_SECONDS).longValue(),
                platformConfig.lookup(FDS_XDI_NFS_STATS).booleanValue(),
                platformConfig.lookup(FDS_XDI_NFS_DEFER_METADATA_UPDATES).booleanValue(),
                platformConfig.lookup(FDS_XDI_NFS_MAX_LIVE_NFS_COOKIES).intValue());
    }
}
