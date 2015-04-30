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

import org.apache.http.client.utils.URIUtils;

import com.formationds.commons.NullArgumentException;

/**
 * General URI utilities.
 */
public final class Uris
{
    /**
     * Get a relative URI from a string path.
     * 
     * @param path A URI.
     * 
     * @return A URI instance.
     * 
     * @throws URISyntaxException when {@code path} is not a valid URI.
     */
    public static URI getRelativeUri(String path) throws URISyntaxException
    {
        if (path == null) throw new NullArgumentException("path");

        return new URI(null, null, null, -1, path, null, null);
    }

    /**
     * Get a hostname verifier that accepts all host names.
     * 
     * @return A new hostname verifier.
     */
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

    /**
     * Get an SSL socket factory that trusts all certificates.
     * 
     * @return A new SSL socket factory.
     */
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

        X509TrustManager trustManager = new X509TrustManager()
        {
            @Override
            public X509Certificate[] getAcceptedIssuers()
            {
                return new X509Certificate[0];
            }

            @Override
            // @eclipseFormat:off
            public void checkClientTrusted(X509Certificate[] chain,
                                           String authType) throws CertificateException
            // @eclispeFormat:on
            {}

            @Override
            // @eclispeFormat:off
            public void checkServerTrusted(X509Certificate[] chain,
                                           String authType) throws CertificateException
            // @eclipseFormat:on
            {}
        };

        try
        {
            sslContext.init(null, new TrustManager[] { trustManager }, null);
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

    /**
     * Determine if one URI is a descendant of another.
     * 
     * @param base The potential parent URI.
     * @param sub The potential child URI.
     * 
     * @return Whether {@code sub} either is, or is underneath {@code base} in a tree structure.
     */
    public static boolean isDescendant(URI base, URI sub)
    {
        if (base == null) throw new NullArgumentException("base");
        if (sub == null) throw new NullArgumentException("sub");

        if (sub.equals(base))
        {
            return true;
        }

        // A relative URI can be a descendant of another relative URI, so we only resolve if the
        // base is absolute and the sub isn't.
        if (base.isAbsolute() && !sub.isAbsolute())
        {
            sub = resolve(base, sub);
        }

        // Get rid of any parent paths.
        URI normalizedBase = base.normalize();
        URI normalizedSub = sub.normalize();

        // Scheme is common to all URIs. This will be null for both if both are relative.
        if (!Strings.nullTolerantEquals(normalizedSub.getScheme(), normalizedBase.getScheme()))
        {
            return false;
        }

        // Opaque URIs can't really be compared, but equal URIs (fragment excepted) are considered
        // descendants of each other (e.g. dir/. == dir).
        if (normalizedBase.isOpaque() && normalizedSub.isOpaque())
        {
            // Scheme-specific part is never null.
            return normalizedBase.getSchemeSpecificPart()
                                 .equals(normalizedSub.getSchemeSpecificPart());
        }

        // Authority must be exactly equal--subdomains aren't descendants of each other, nor are
        // different ports on the same host, or different user logins to the same endpoint. This
        // also covers registry-based URIs (authority is opaque).
        if (!Strings.nullTolerantEquals(normalizedSub.getAuthority(), normalizedBase.getAuthority()))
        {
            return false;
        }

        // All components after the path are ignored.
        return Strings.nullTolerantStartsWith(normalizedSub.getPath(), normalizedBase.getPath());
    }

    /**
     * Resolve a relative URI against an absolute base.
     * 
     * @param base The base URI.
     * @param relative The relative URI.
     * 
     * @return The resolved URI.
     */
    public static URI resolve(URI base, URI relative)
    {
        if (base == null) throw new NullArgumentException("base");
        if (relative == null) throw new NullArgumentException("relative");
        if (relative.isAbsolute())
        {
            throw new IllegalArgumentException("relative is not a relative URI: "
                                               + relative.toString());
        }

        return URIUtils.resolve(base, relative);
    }

    /**
     * Try to get a path as a relative URI.
     * 
     * @param path The relative URI path.
     * 
     * @return A URI, or {@code null} if {@code path} was not a valid relative URI.
     */
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

    /**
     * Add query parameters to a URI.
     * 
     * @param base The base URI.
     * @param key The query parameter key.
     * @param value The query parameter value.
     * 
     * @return {@code base} with the added query parameters.
     * 
     * @throws URISyntaxException when {@code key} or {@code value} contain invalid characters.
     */
    public static URI
            withQueryParameters(URI base, String key, String value) throws URISyntaxException
    {
        if (base == null) throw new NullArgumentException("base");
        if (key == null) throw new NullArgumentException("key");
        if (value == null) throw new NullArgumentException("value");

        return withQueryParameters(base, key + "=" + value);
    }

    /**
     * Add query parameters to a URI.
     * 
     * @param base The base URI.
     * @param key1 The first query parameter key.
     * @param value1 The first query parameter value.
     * @param key2 The second query parameter key.
     * @param value2 The second query parameter value.
     * 
     * @return {@code base} with the added query parameters.
     * 
     * @throws URISyntaxException when any key or value contains invalid characters.
     */
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

    /**
     * Add query parameters to a URI.
     * 
     * @param base The base URI.
     * @param parameters The parameters to add.
     * 
     * @return {@code base} with the added query parameters.
     * 
     * @throws URISyntaxException when any key or value contains invalid characters.
     */
    @SafeVarargs
    // @eclipseFormat:off
    public static URI withQueryParameters(URI base,
                                          AbstractMap.SimpleEntry<String, String>... parameters)
          throws URISyntaxException
    // @eclipseFormat:on
    {
        if (base == null) throw new NullArgumentException("base");
        if (parameters == null) throw new NullArgumentException("parameters");

        return withQueryParameters(base, Arrays.asList(parameters));
    }

    /**
     * Join a query parameter key/value pair.
     * 
     * @param parameter The pair to join.
     * 
     * @return key=value
     */
    public static String joinParameterPair(AbstractMap.SimpleEntry<String, String> parameter)
    {
        if (parameter == null) throw new NullArgumentException("parameter");

        final String key = parameter.getKey();
        final String value = parameter.getValue();

        if (key == null) throw new IllegalArgumentException("parameter has a null key.");
        if (value == null) throw new IllegalArgumentException("parameter has a null value.");

        return key + "=" + value;
    }

    /**
     * Add query parameters to a URI.
     * 
     * @param base The base URI.
     * @param parameters The parameters to add.
     * 
     * @return {@code base} with the added query parameters.
     * 
     * @throws URISyntaxException when any key or value contains invalid characters.
     */
    // @eclipseFormat:off
    public static URI withQueryParameters(URI base,
                                          Iterable<AbstractMap.SimpleEntry<String,
                                                                           String>> parameters)
            throws URISyntaxException
    // @eclipseFormat:on
    {
        if (base == null) throw new NullArgumentException("base");
        if (parameters == null) throw new NullArgumentException("parameters");

        UriBuilder builder = UriBuilder.fromUri(base);

        for (AbstractMap.SimpleEntry<String, String> parameter : parameters)
        {
            builder.queryParam(parameter.getKey(), parameter.getValue());
        }

        return builder.build();
    }

    /**
     * Set the query string on a URI.
     * 
     * @param base The base URI.
     * @param query The replacement query string.
     * 
     * @return {@code base} with the new query string.
     * 
     * @throws URISyntaxException when there are invalid characters in {@code query}.
     */
    public static URI withQueryParameters(URI base, String query) throws URISyntaxException
    {
        if (base == null) throw new NullArgumentException("base");
        if (query == null) throw new NullArgumentException("query");

        // return base.resolve(new URI(null, null, null, -1, null, query, null));
        return UriBuilder.fromUri(base).replaceQuery(query).build();
    }

    /**
     * Prevent instantiation.
     */
    private Uris()
    {
        throw new UnsupportedOperationException("Instantiating a utility class.");
    }
}
