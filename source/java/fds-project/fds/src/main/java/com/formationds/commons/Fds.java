package com.formationds.commons;

import com.formationds.commons.annotations.Replacement;
import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.commons.util.Strings;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.ConfigUndefinedException;
import com.formationds.util.Configuration;

import java.net.URI;
import java.net.URISyntaxException;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * Global FDS configuration.
 */
public final class Fds
{
    /**
     * API pseudo-constants.
     */
    public static class Api
    {
        /**
         * Version 7 of the API.
         */
        @Deprecated
        public static class V07 extends Api { }

        /**
         * Version 8 of the API.
         */
        public static class V08
        {
            /**
             * URL to get an auth token.
             *
             * @return A URL.
             */
            public static URI getAuthToken()
            {
                final URI apiBase = V08.getBase();
                final URI authTokenPath = Uris.tryGetRelativeUri("token/");

                return Uris.resolve(apiBase, authTokenPath);
            }

            /**
             * API base URL.
             *
             * @return A URL.
             */
            public static URI getBase()
            {
                final String scheme = "https";
                final String host = getFdsHost();
                final int omPort = getOmPort();
                final String path = "/fds/config/v08/";

                try
                {
                    return new URI(scheme, null, host, omPort, path, null, null);
                }
                catch (URISyntaxException e)
                {
                    throw newUriConstructionException(scheme, null, host, omPort, path);
                }
            }

            /**
             * URL to get system volumes.
             *
             * @return A URL.
             */
            public static URI getVolumes()
            {
                final URI apiBase = V08.getBase();
                final URI volumesPath = Uris.tryGetRelativeUri("volumes/");

                return Uris.resolve(apiBase, volumesPath);
            }
        }

        /**
         * The URI to access to get an authentication token.
         * 
         * @return The auth token URI.
         */
        @Replacement("API_AUTH_TOKEN")
        public static URI getAuthToken()
        {
            final URI apiBase = Api.getBase();
            final URI authTokenPath = Uris.tryGetRelativeUri("auth/token/");

            return Uris.resolve(apiBase, authTokenPath);
        }

        /**
         * OM base URI.
         * 
         * @return The URI.
         */
        @Replacement("API_BASE")
        public static URI getBase()
        {
            final String scheme = "https";
            final String host = getFdsHost();
            final int omPort = getOmPort();
            final String path = "/api/";

            try
            {
                return new URI(scheme, null, host, omPort, path, null, null);
            }
            catch (URISyntaxException e)
            {
                throw newUriConstructionException(scheme, null, host, omPort, path);
            }
        }

        /**
         * OM volume list URI.
         * 
         * @return The URI.
         */
        @Replacement("API_VOLUMES")
        public static URI getVolumes()
        {
            final URI apiBase = Api.getBase();
            final URI volumesPath = Uris.tryGetRelativeUri("config/volumes/");

            return Uris.resolve(apiBase, volumesPath);
        }

        /**
         * Prevent instantiation.
         */
        private Api()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }
    }

    /**
     * Configuration values.
     */
    public final static class Config
    {
        /**
         * System-wide I/O throttle.
         */
        @Replacement("DISK_IOPS_MAX_CONFIG")
        public static final String DISK_IOPS_MAX_CONFIG;

        /**
         * Guaranteed system-wide IOPS.
         */
        @Replacement("DISK_IOPS_MIN_CONFIG")
        public static final String DISK_IOPS_MIN_CONFIG;

        /**
         * Constructor.
         * 
         * @param commandName The name of the currently running process.
         * @param args Command-line arguments this process was invoked with.
         */
        public Config(String commandName, String[] args)
        {
            if (commandName == null) throw new NullArgumentException("commandName");
            if (args == null) throw new NullArgumentException("args");

            _configuration = new Configuration(commandName, args);
            _fdsRoot = null;
            _iopsMax = -1;
            _iopsMin = -1;
            _platformConfig = null;
        }

        /**
         * Root path for FDS data.
         * 
         * @return A path.
         */
        @Replacement("FDS_ROOT")
        public Path getFdsRoot()
        {
            if (_fdsRoot == null)
            {
                Configuration configuration = getConfiguration();
                _fdsRoot = Paths.get(configuration.getFdsRoot());
            }
            return _fdsRoot;
        }

        /**
         * System-wide I/O throttle.
         * 
         * @return The maximum IOPS allowed.
         * 
         * @throws ConfigUndefinedException when this configuration option is not available.
         */
        @Replacement("IOPS_MAX")
        public int getIopsMax() throws ConfigUndefinedException
        {
            if (_iopsMax < 0)
            {
                ParsedConfig platformConfig = getPlatformConfig();
                _iopsMax = platformConfig.defaultInt(DISK_IOPS_MAX_CONFIG, -1);
                if (_iopsMax < 0)
                {
                    throw new ConfigUndefinedException("Could not find "
                                                       + Strings.javaString(DISK_IOPS_MAX_CONFIG)
                                                       + " in config file "
                                                       + Strings.javaString(getPlatformConfigPath().toAbsolutePath()
                                                                                                   .toString()));
                }
            }
            return _iopsMax;
        }

        /**
         * Guaranteed system IOPS.
         * 
         * @return The minimum IOPS that will always be available.
         * 
         * @throws ConfigUndefinedException when this configuration option is not available.
         */
        @Replacement("IOPS_MIN")
        public int getIopsMin() throws ConfigUndefinedException
        {
            if (_iopsMin < 0)
            {
                ParsedConfig platformConfig = getPlatformConfig();
                _iopsMin = platformConfig.defaultInt(DISK_IOPS_MIN_CONFIG, -1);
                if (_iopsMin < 0)
                {
                    throw new ConfigUndefinedException("Could not find "
                                                       + Strings.javaString(DISK_IOPS_MIN_CONFIG)
                                                       + " in config file "
                                                       + Strings.javaString(getPlatformConfigPath().toAbsolutePath()
                                                                                                   .toString()));
                }
            }
            return _iopsMin;
        }

        /**
         * Get the path to platform.conf.
         * 
         * @return A path.
         */
        public Path getPlatformConfigPath()
        {
            return getConfiguration().getPlatformConfigPath();
        }

        /**
         * Runtime configuration.
         */
        private final Configuration _configuration;

        /**
         * The root of the FDS installation.
         */
        private Path _fdsRoot;

        /**
         * System I/O throttle.
         */
        private int _iopsMax;

        /**
         * Guaranteed system IOPS.
         */
        private int _iopsMin;

        /**
         * Runtime platform.conf.
         */
        private ParsedConfig _platformConfig;

        /**
         * Get the runtime configuration.
         * 
         * @return The configuration.
         */
        private Configuration getConfiguration()
        {
            return _configuration;
        }

        /**
         * Get the platform.conf.
         * 
         * @return Parsed platform.conf.
         */
        private ParsedConfig getPlatformConfig()
        {
            if (_platformConfig == null)
            {
                _platformConfig = getConfiguration().getPlatformConfig();
            }
            return _platformConfig;
        }

        /**
         * Static constructor.
         */
        static
        {
            final String testingConfig = "fds.pm.testing";

            DISK_IOPS_MAX_CONFIG = testingConfig + ".node_iops_max";
            DISK_IOPS_MIN_CONFIG = testingConfig + ".node_iops_min";
        }
    }

    /**
     * Centralized random number generation adds unpredictability, as well as minimizing the number
     * of places something can be done wrong.
     */
    public static final class Random
    {
        /**
         * Generate random bytes.
         * 
         * @param bytes Fill this array.
         * 
         * @see Random#nextBytes(byte[]).
         */
        public static void nextBytes(byte[] bytes)
        {
            if (bytes == null) throw new NullArgumentException("bytes");
            
            _random.nextBytes(bytes);
        }
        
        /**
         * Generate a random number.
         * 
         * @param minInclusive Returned value will be greater than or equal to this.
         * @param maxExclusive Returned value will be less than this.
         * 
         * @return A random number.
         */
        public static int nextInt(int minInclusive, int maxExclusive)
        {
            if (minInclusive < 0)
            {
                throw new IllegalArgumentException("minInclusive must be >= 0. Received "
                                                   + minInclusive + ".");
            }
            if (maxExclusive <= minInclusive)
            {
                throw new IllegalArgumentException("Arguments must be minInclusive < maxExclusive. Received "
                                                   + minInclusive + ", " + maxExclusive + ".");
            }

            return minInclusive + nextInt(maxExclusive - minInclusive);
        }
        
        /**
         * Generate a random number.
         * 
         * @param bound Returned value will be less than this.
         * 
         * @return A 0-based random number.
         */
        public static int nextInt(int bound)
        {
            return _random.nextInt(bound);
        }

        /**
         * Internal random number generator.
         */
        private static final java.util.Random _random;

        /**
         * Prevent instantiation.
         */
        private Random()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        /**
         * Static constructor.
         */
        static
        {
            _random = new java.util.Random();
        }
    }
    
    /**
     * Centralized secure random number generation.
     */
    public static final class SecureRandom
    {
        /**
         * Generate secure random bytes.
         * 
         * @param bytes Fill this array.
         */
        public static void nextBytes(byte[] bytes)
        {
            if (bytes == null) throw new NullArgumentException("bytes");
            
            _secureRandom.nextBytes(bytes);
        }
        
        /**
         * Internal secure random number generator.
         */
        private static final java.security.SecureRandom _secureRandom;
        
        /**
         * Prevent instantiation.
         */
        private SecureRandom()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        /**
         * Static constructor.
         */
        static
        {
            _secureRandom = new java.security.SecureRandom();
        }
    }

    /**
     * The HTTP header the FDS auth token should be placed in.
     */
    @Replacement("FDS_AUTH_HEADER")
    public static final String FDS_AUTH_HEADER;

    /**
     * The hostname of the primary FDS endpoint.
     */
    @Replacement("FDS_HOST_PROPERTY")
    public static final String FDS_HOST_PROPERTY;

    /**
     * The name of the query parameter the password is sent in.
     */
    @Replacement("PASSWORD_QUERY_PARAMETER")
    public static final String PASSWORD_QUERY_PARAMETER;

    /**
     * The name of the query parameter the username is sent in.
     */
    @Replacement("USERNAME_QUERY_PARAMETER")
    public static final String USERNAME_QUERY_PARAMETER;

    /**
     * Get the primary FDS host.
     * 
     * @return A hostname.
     */
    @Replacement("FDS_HOST")
    public static String getFdsHost()
    {
        return System.getProperty(FDS_HOST_PROPERTY, "localhost");
    }

    /**
     * Get the primary FDS port.
     * 
     * @return A TCP port.
     */
    @Replacement("OM_PORT")
    public static int getOmPort()
    {
        // FIXME: This is calculated. See
        //        https://github.com/FDS-Dev/fds-src/commit/5954882bd7f47854a0f32588822ebb160c743532
        return 7443;
    }

    /**
     * Get the S3 endpoint for this cluster.
     * 
     * @return An S3 endpoint.
     */
    @Replacement("S3_ENDPOINT")
    public static URI getS3Endpoint()
    {
        final String scheme = "https";
        final String host = getFdsHost();
        final int s3Port = getS3Port();

        try
        {
            return new URI(scheme, null, host, s3Port, null, null, null);
        }
        catch (URISyntaxException e)
        {
            throw newUriConstructionException(scheme, null, host, s3Port, null);
        }
    }

    /**
     * Get the port the S3 service is running on.
     * 
     * @return A TCP port.
     */
    @Replacement("S3_PORT")
    public static int getS3Port()
    {
        return 8443;
    }

    /**
     * Static constructor.
     */
    static
    {
        FDS_AUTH_HEADER = "FDS-Auth";
        FDS_HOST_PROPERTY = "fds.host";
        PASSWORD_QUERY_PARAMETER = "password";
        USERNAME_QUERY_PARAMETER = "login";
    }

    /**
     * Make a new exception describing a URI construction error.
     * 
     * @param scheme Scheme for the URI.
     * @param userInfo Userinfo section of the URI.
     * @param host Host for the URI.
     * @param port Port for the URI.
     * @param path Path portion of the URI.
     * 
     * @return An exception.
     */
    private static IllegalStateException newUriConstructionException(String scheme,
                                                                     String userInfo,
                                                                     String host,
                                                                     int port,
                                                                     String path)
    {
        return newUriConstructionException(scheme,
                                           userInfo,
                                           host,
                                           port,
                                           path,
                                           null,
                                           null,
                                           null);
    }

    /**
     * Make a new exception describing a URI construction error.
     * 
     * @param scheme Scheme for the URI.
     * @param userInfo Userinfo section of the URI.
     * @param host Host for the URI.
     * @param port Port for the URI.
     * @param path Path for the URI.
     * @param query Query portion of the URI.
     * @param fragment Fragment portion of the URI.
     * @param cause Proximate cause for the error to be returned.
     * 
     * @return An exception.
     */
    private static IllegalStateException newUriConstructionException(String scheme,
                                                                     String userInfo,
                                                                     String host,
                                                                     int port,
                                                                     String path,
                                                                     String query,
                                                                     String fragment,
                                                                     Throwable cause)
    {
        return new IllegalStateException("Error constructing URI("
                                         + String.join(", ",
                                                       Strings.javaString(scheme),
                                                       Strings.javaString(userInfo),
                                                       Strings.javaString(host),
                                                       Integer.toString(port),
                                                       Strings.javaString(path),
                                                       Strings.javaString(query),
                                                       Strings.javaString(fragment)) + ")", cause);
    }
}
