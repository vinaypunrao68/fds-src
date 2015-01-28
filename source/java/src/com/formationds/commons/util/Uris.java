package com.formationds.commons.util;

import java.net.URI;
import java.net.URISyntaxException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.AbstractMap;
import java.util.Arrays;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import javax.ws.rs.core.UriBuilder;

import com.formationds.iodriver.NullArgumentException;

public final class Uris
{
    public static URI getRelativeUri(String path) throws URISyntaxException
    {
        if (path == null) throw new NullArgumentException("path");

        return new URI(null, null, null, -1, path, null, null);
    }

    public static HostnameVerifier getTrustingHostnameVerifier()
    {
        return new HostnameVerifier()
        {
            @Override
            public boolean verify(String hostname, SSLSession session)
            {
                return true;
            }
        };
    }
    
    public static SSLSocketFactory getTrustingSocketFactory()
    {
        SSLContext sslContext;
        try
        {
            sslContext = SSLContext.getInstance("TLSv1.2");
        }
        catch (NoSuchAlgorithmException e)
        {
            // This should be impossible.
            throw new RuntimeException(e);
        }

        try
        {
            sslContext.init(null, new TrustManager[]
            {
                            new X509TrustManager()
                            {
                                @Override
                                public X509Certificate[] getAcceptedIssuers()
                                {
                                    return new X509Certificate[0];
                                }

                                @Override
                                public void
                                        checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException
                                {}

                                @Override
                                public void
                                        checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException
                                {}
                            }
            },
                            null);
        }
        catch (KeyManagementException e)
        {
            // This probably should be impossible or next to it since we're using the default key
            // manager, but the documentation says the exception is thrown "when this operation
            // fails", so who knows. A checked exception shouldn't be that vague.
            throw new RuntimeException(e);
        }

        return sslContext.getSocketFactory();
    }

    public static URI tryGetRelativeUri(String path)
    {
        try
        {
            return getRelativeUri(path);
        }
        catch (URISyntaxException e)
        {
            return null;
        }
    }

    public static URI
            withQueryParameters(URI base, String key, String value) throws URISyntaxException
    {
        if (base == null) throw new NullArgumentException("base");
        if (key == null) throw new NullArgumentException("key");
        if (value == null) throw new NullArgumentException("value");

        return withQueryParameters(base, key + "=" + value);
    }

    public static URI withQueryParameters(URI base,
                                          String key1,
                                          String value1,
                                          String key2,
                                          String value2) throws URISyntaxException
    {
        if (base == null) throw new NullArgumentException("base");
        if (key1 == null) throw new NullArgumentException("key1");
        if (value1 == null) throw new NullArgumentException("value1");
        if (key2 == null) throw new NullArgumentException("key2");
        if (value2 == null) throw new NullArgumentException("value2");

        return withQueryParameters(base,
                                   new AbstractMap.SimpleEntry<String, String>(key1, value1),
                                   new AbstractMap.SimpleEntry<String, String>(key2, value2));
    }

    @SafeVarargs
    public static URI
            withQueryParameters(URI base, AbstractMap.SimpleEntry<String, String>... parameters) throws URISyntaxException
    {
        if (base == null) throw new NullArgumentException("base");
        if (parameters == null) throw new NullArgumentException("parameters");

        return withQueryParameters(base, Arrays.asList(parameters));
    }

    public static String joinParameterPair(AbstractMap.SimpleEntry<String, String> parameter)
    {
        if (parameter == null) throw new NullArgumentException("parameter");

        final String key = parameter.getKey();
        final String value = parameter.getValue();

        if (key == null) throw new IllegalArgumentException("parameter has a null key.");
        if (value == null) throw new IllegalArgumentException("parameter has a null value.");

        return key + "=" + value;
    }

    public static URI
            withQueryParameters(URI base,
                                Iterable<AbstractMap.SimpleEntry<String, String>> parameters) throws URISyntaxException
    {
        if (base == null) throw new NullArgumentException("base");
        if (parameters == null) throw new NullArgumentException("parameters");

        UriBuilder builder = UriBuilder.fromUri(base);

        for (AbstractMap.SimpleEntry<String, String> parameter : parameters)
        {
            builder.queryParam(parameter.getKey(), parameter.getValue());
        }

        // Stream<AbstractMap.SimpleEntry<String, String>> parametersAsStream =
        // StreamSupport.stream(parameters.spliterator(), false);
        // Stream<String> parametersAsKvp = parametersAsStream.map(Uris::joinParameterPair);
        // String[] parametersAsStringArray = parametersAsKvp.toArray(size -> new String[size]);
        // List<String> parametersAsList = Arrays.asList(parametersAsStringArray);
        // String parametersAsString = String.join("&", parametersAsList);
        //
        // return base.resolve(new URI(null, null, null, -1, null, parametersAsString, null));
        return builder.build();
    }

    public static URI withQueryParameters(URI base, String query) throws URISyntaxException
    {
        if (base == null) throw new NullArgumentException("base");
        if (query == null) throw new NullArgumentException("query");

        // return base.resolve(new URI(null, null, null, -1, null, query, null));
        return UriBuilder.fromUri(base).replaceQuery(query).build();
    }

    private Uris()
    {
        throw new UnsupportedOperationException("Instantiating a utility class.");
    }
}
