package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSocketFactory;

import com.formationds.commons.util.Uris;
import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.logging.Logger;

public class HttpsEndpoint extends HttpEndpoint
{
    public HttpsEndpoint(URI uri, Logger logger) throws MalformedURLException
    {
        this(toUrl(uri), logger);
    }
    
    public HttpsEndpoint(URI uri, Logger logger, boolean trusting) throws MalformedURLException
    {
        this(toUrl(uri), logger, trusting);
    }
    
    public HttpsEndpoint(URL url, Logger logger)
    {
        this(url, logger, false);
    }
    
    public HttpsEndpoint(URL url, Logger logger, boolean trusting)
    {
        super(url, logger);
        
        String scheme = url.getProtocol();
        if (scheme == null || !scheme.equalsIgnoreCase("https"))
        {
            throw new IllegalArgumentException("url is not HTTPS: " + url);
        }
        
        _trusting = trusting;
    }
    
    public HttpsEndpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        return new HttpsEndpoint(copyHelper.url, copyHelper.logger, copyHelper.trusting);
    }
    
    public boolean isTrusting()
    {
        return _trusting;
    }

    protected class CopyHelper extends HttpEndpoint.CopyHelper
    {
        public final boolean trusting = _trusting;
    }
    
    protected HttpsEndpoint(CopyHelper helper)
    {
        super(helper);
        
        _trusting = helper.trusting;
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

    private final boolean _trusting;

    private static final HostnameVerifier _trustingHostnameVerifier;
    
    private static final SSLSocketFactory _trustingSocketFactory;
    
    static
    {
        _trustingHostnameVerifier = Uris.getTrustingHostnameVerifier();
        _trustingSocketFactory = Uris.getTrustingSocketFactory();
    }
}
