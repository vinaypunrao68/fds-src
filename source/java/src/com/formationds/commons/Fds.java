package com.formationds.commons;

import java.net.URI;
import java.net.URISyntaxException;
import java.nio.file.Path;
import java.nio.file.Paths;

import com.formationds.commons.annotations.Replacement;
import com.formationds.commons.util.Strings;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.ConfigUndefinedException;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;

// FIXME: Make a better name.
public final class Fds
{
    public final static class Api
    {
        @Replacement("API_AUTH_TOKEN")
        public static URI getAuthToken()
        {
            final URI apiBase = Api.getBase();
            final URI authTokenPath = Uris.tryGetRelativeUri("auth/token/");

            return Uris.resolve(apiBase, authTokenPath);
        }

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

        @Replacement("API_VOLUMES")
        public static URI getVolumes()
        {
            final URI apiBase = Api.getBase();
            final URI volumesPath = Uris.tryGetRelativeUri("config/volumes/");

            return Uris.resolve(apiBase, volumesPath);
        }

        private Api()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }
    }

    public final static class Config
    {
        @Replacement("DISK_IOPS_MAX_CONFIG")
        public static final String DISK_IOPS_MAX_CONFIG;

        @Replacement("DISK_IOPS_MIN_CONFIG")
        public static final String DISK_IOPS_MIN_CONFIG;

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

        public Path getPlatformConfigPath()
        {
            return getConfiguration().getPlatformConfigPath();
        }

        private final Configuration _configuration;

        private Path _fdsRoot;

        private int _iopsMax;

        private int _iopsMin;

        private ParsedConfig _platformConfig;

        private Configuration getConfiguration()
        {
            return _configuration;
        }

        private ParsedConfig getPlatformConfig()
        {
            if (_platformConfig == null)
            {
                _platformConfig = getConfiguration().getPlatformConfig();
            }
            return _platformConfig;
        }

        static
        {
            final String testingConfig = "fds.plat.testing";

            DISK_IOPS_MAX_CONFIG = testingConfig + ".disk_iops_max";
            DISK_IOPS_MIN_CONFIG = testingConfig + ".disk_iops_min";
        }
    }
    
    // It makes sense to centralize random processing--as there's more and more independent
    // consumers, it becomes more difficult to observe, predict, or have any control.
    public static final class Random
    {
        public static int nextInt(int bound)
        {
            return _random.nextInt(bound);
        }
        
        private static final java.util.Random _random;
        
        private Random()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }
        
        static
        {
            _random = new java.util.Random();
        }
    }

    @Replacement("FDS_AUTH_HEADER")
    public static final String FDS_AUTH_HEADER;

    @Replacement("FDS_HOST_PROPERTY")
    public static final String FDS_HOST_PROPERTY;

    @Replacement("PASSWORD_QUERY_PARAMETER")
    public static final String PASSWORD_QUERY_PARAMETER;

    @Replacement("USERNAME_QUERY_PARAMETER")
    public static final String USERNAME_QUERY_PARAMETER;

    @Replacement("FDS_HOST")
    public static String getFdsHost()
    {
        return System.getProperty(FDS_HOST_PROPERTY, "localhost");
    }

    @Replacement("OM_PORT")
    public static int getOmPort()
    {
        return 7443;
    }

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

    @Replacement("S3_PORT")
    public static int getS3Port()
    {
        return 8443;
    }

    static
    {
        FDS_AUTH_HEADER = "FDS-Auth";
        FDS_HOST_PROPERTY = "fds.host";
        PASSWORD_QUERY_PARAMETER = "password";
        USERNAME_QUERY_PARAMETER = "login";
    }

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
