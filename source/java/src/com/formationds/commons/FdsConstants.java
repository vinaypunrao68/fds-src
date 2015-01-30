package com.formationds.commons;

import java.net.URI;
import java.net.URISyntaxException;

import com.formationds.commons.annotations.Replacement;
import com.formationds.commons.util.Strings;
import com.formationds.commons.util.Uris;

// FIXME: Make a better name.
public final class FdsConstants
{
    public final static class Api
    {
        @Replacement("API_AUTH_TOKEN")
        public static URI getAuthToken()
        {
            final URI apiBase = Api.getBase();
            final URI authTokenPath = Uris.tryGetRelativeUri("auth/token");

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
            final URI volumesPath = Uris.tryGetRelativeUri("config/volumes");
            
            return Uris.resolve(apiBase, volumesPath);
        }
        
        private Api()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
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
