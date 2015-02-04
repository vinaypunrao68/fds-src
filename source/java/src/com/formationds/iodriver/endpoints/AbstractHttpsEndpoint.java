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
import com.formationds.iodriver.reporters.WorkflowEventListener;

// @eclipseFormat:off
public abstract class AbstractHttpsEndpoint<
        ThisT extends AbstractHttpsEndpoint<ThisT, OperationT>,
        OperationT extends AbstractHttpOperation<OperationT, ThisT>>
extends AbstractHttpEndpoint<ThisT, OperationT>
// @eclipseFormat:on
{
    public AbstractHttpsEndpoint(URI uri, Logger logger) throws MalformedURLException
    {
        this(toUrl(uri), logger);
    }

    public AbstractHttpsEndpoint(URI uri,
                                 Logger logger,
                                 boolean trusting) throws MalformedURLException
    {
        this(toUrl(uri), logger, trusting);
    }

    public AbstractHttpsEndpoint(URL url, Logger logger)
    {
        this(url, logger, false);
    }

    public AbstractHttpsEndpoint(URL url,
                                 Logger logger,
                                 boolean trusting)
    {
        super(url, logger);

        String scheme = url.getProtocol();
        if (scheme == null || !scheme.equalsIgnoreCase("https"))
        {
            throw new IllegalArgumentException("url is not HTTPS: " + url);
        }

        _trusting = trusting;
    }

    public boolean isTrusting()
    {
        return _trusting;
    }

    protected class CopyHelper extends AbstractHttpEndpoint<ThisT, OperationT>.CopyHelper
    {
        public final boolean trusting = _trusting;
    }

    protected AbstractHttpsEndpoint(CopyHelper helper)
    {
        super(helper);

        _trusting = helper.trusting;
    }

    protected HttpsURLConnection openConnection() throws IOException
    {
        return (HttpsURLConnection)super.openConnection();
    }

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

    protected HttpsURLConnection openRelativeConnection(URI relativeUri) throws IOException
    {
        if (relativeUri == null) throw new NullArgumentException("relativeUri");

        return (HttpsURLConnection)super.openRelativeConnection(relativeUri);
    }

    private final boolean _trusting;

    private static final HostnameVerifier _trustingHostnameVerifier;

    private static final SSLSocketFactory _trustingSocketFactory;

    static
    {
        _trustingHostnameVerifier = Uris.getTrustingHostnameVerifier();
        _trustingSocketFactory = Uris.getTrustingSocketFactory();
    }
}
