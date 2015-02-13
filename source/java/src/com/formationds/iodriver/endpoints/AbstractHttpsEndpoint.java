package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSocketFactory;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.AbstractHttpOperation;

/**
 * Endpoint that only accepts HTTPS connections.
 * 
 * @param <ThisT> The implementing class.
 * @param <OperationT> Type of operations that may be visited.
 */
// @eclipseFormat:off
public abstract class AbstractHttpsEndpoint<
        ThisT extends AbstractHttpsEndpoint<ThisT, OperationT>,
        OperationT extends AbstractHttpOperation<OperationT, ThisT>>
extends AbstractHttpEndpoint<ThisT, OperationT>
// @eclipseFormat:on
{
    /**
     * Constructor.
     * 
     * @param uri Base URI for requests. Must be a valid absolute URL.
     * @param logger Log target.
     * 
     * @throws MalformedURLException when {@code uri} is not a valid absolute URL.
     */
    public AbstractHttpsEndpoint(URI uri, Logger logger) throws MalformedURLException
    {
        this(toUrl(uri), logger);
    }

    /**
     * Constructor.
     * 
     * @param uri Base URI for requests. Must be a valid absolute URL>
     * @param logger Log target.
     * @param trusting Whether normally-untrusted certificates are accepted.
     * 
     * @throws MalformedURLException when {@code uri} is not a valid absolute URL.
     */
    public AbstractHttpsEndpoint(URI uri,
                                 Logger logger,
                                 boolean trusting) throws MalformedURLException
    {
        this(toUrl(uri), logger, trusting);
    }

    /**
     * Constructor.
     * 
     * @param url Base URL for requests.
     * @param logger Log target.
     */
    public AbstractHttpsEndpoint(URL url, Logger logger)
    {
        this(url, logger, false);
    }

    /**
     * Constructor.
     * 
     * @param url Base URL for requests.
     * @param logger Log target.
     * @param trusting Whether normally-untrusted certificates are accepted.
     */
    public AbstractHttpsEndpoint(URL url, Logger logger, boolean trusting)
    {
        super(url, logger);

        String scheme = url.getProtocol();
        if (scheme == null || !scheme.equalsIgnoreCase("https"))
        {
            throw new IllegalArgumentException("url is not HTTPS: " + url);
        }

        _trusting = trusting;
    }

    /**
     * Get whether normally-untrusted (self-signed, host mismatch, unrecognized authority)
     * certificates are accepted.
     * 
     * @return The current property value.
     */
    public boolean isTrusting()
    {
        return _trusting;
    }

    /**
     * Extend this class to allow deep copies even when superclass private members aren't available.
     */
    protected class CopyHelper extends AbstractHttpEndpoint<ThisT, OperationT>.CopyHelper
    {
        /**
         * Whether normally-untrusted certificates should be accepted.
         */
        public final boolean trusting = _trusting;
    }

    /**
     * Copy constructor.
     * 
     * @param helper Object holding copied values to assign to the new object.
     */
    protected AbstractHttpsEndpoint(CopyHelper helper)
    {
        super(helper);

        _trusting = helper.trusting;
    }

    @Override
    protected HttpsURLConnection openConnection() throws IOException
    {
        return (HttpsURLConnection)super.openConnection();
    }

    @Override
    protected URLConnection openConnection(URL url) throws IOException
    {
        if (url == null) throw new NullArgumentException("url");

        URLConnection connection = super.openConnection(url);

        if (isTrusting())
        {
            if (connection instanceof HttpsURLConnection)
            {
                HttpsURLConnection typedConnection = (HttpsURLConnection)connection;

                typedConnection.setSSLSocketFactory(_trustingSocketFactory);
                typedConnection.setHostnameVerifier(_trustingHostnameVerifier);
            }
        }

        return connection;
    }

    @Override
    protected HttpsURLConnection openRelativeConnection(URI relativeUri) throws IOException
    {
        if (relativeUri == null) throw new NullArgumentException("relativeUri");

        return (HttpsURLConnection)super.openRelativeConnection(relativeUri);
    }

    /**
     * Whether nomally-untrusted certificates are to be accepted.
     */
    private final boolean _trusting;

    /**
     * Accepts all hostnames.
     */
    private static final HostnameVerifier _trustingHostnameVerifier;

    /**
     * Accepts all certificates.
     */
    private static final SSLSocketFactory _trustingSocketFactory;

    static
    {
        _trustingHostnameVerifier = Uris.getTrustingHostnameVerifier();
        _trustingSocketFactory = Uris.getTrustingSocketFactory();
    }
}
