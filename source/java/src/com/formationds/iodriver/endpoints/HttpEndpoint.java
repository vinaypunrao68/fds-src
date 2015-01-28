package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

import javax.net.ssl.SSLSocketFactory;

import com.formationds.commons.util.Strings;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.HttpOperation;
import com.formationds.iodriver.operations.Operation;
import com.google.common.io.CharStreams;
import com.google.common.net.MediaType;

public class HttpEndpoint extends Endpoint
{
    public HttpEndpoint(URI uri, Logger logger) throws MalformedURLException
    {
        this(toUrl(uri), logger);
    }

    public HttpEndpoint(URL url, Logger logger)
    {
        if (url == null) throw new NullArgumentException("url");
        {
            String scheme = url.getProtocol();
            if (scheme == null
                || !(scheme.equalsIgnoreCase("http") || scheme.equalsIgnoreCase("https")))
            {
                throw new IllegalArgumentException("url is not HTTP or HTTPS: " + url);
            }
        }
        if (logger == null) throw new NullArgumentException("logger");

        _logger = logger;
        _url = url;
    }

    @Override
    public HttpEndpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        return new HttpEndpoint(copyHelper);
    }

    @Override
    public void exec(Operation operation) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (!(operation instanceof HttpOperation))
        {
            throw new IllegalArgumentException("operation must be an instance of "
                                               + HttpOperation.class.getName());
        }

        exec((HttpOperation)operation);
    }

    public void exec(HttpOperation operation) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");

        HttpURLConnection connection;
        try
        {
            connection = openConnection();
        }
        catch (IOException e)
        {
            throw new ExecutionException(e);
        }
        
        try
        {
            operation.exec(connection);
        }
        finally
        {
            connection.disconnect();
        }
    }

    public String getContent(HttpURLConnection connection) throws HttpException
    {
        int responseCode = -2;
        try
        {
            responseCode = connection.getResponseCode();
        }
        catch (IOException e)
        {
            throw new HttpException("Error retrieving response code.", e);
        }
        if (responseCode < 0)
        {
            throw new HttpException(responseCode);
        }
        int responseClass = responseCode / 100;

        String responseMessage;
        try
        {
            responseMessage = connection.getResponseMessage();
        }
        catch (IOException e)
        {
            throw new HttpException("Error retrieving response message.", e);
        }

        switch (responseClass)
        {
        case 2:
            return getResponse(connection);
        case 4:
            throw new HttpClientException(responseCode,
                                          responseMessage,
                                          getErrorResponse(connection));
        case 5:
            throw new HttpServerException(responseCode,
                                          responseMessage,
                                          getErrorResponse(connection));
        default:
            throw new HttpException(responseCode, responseMessage);
        }
    }

    public URL getUrl()
    {
        try
        {
            return new URL(_url.toString());
        }
        catch (MalformedURLException e)
        {
            // This should be impossible.
            throw new RuntimeException("Unexpected error making defensive copy of _url.", e);
        }
    }
    
    protected class CopyHelper extends Endpoint.CopyHelper
    {
        public final Logger logger = _logger;
        public final URL url = _url;
    }

    protected HttpEndpoint(CopyHelper helper)
    {
        super(helper);
        
        _logger = helper.logger;
        _url = helper.url;
    }
    
    protected Charset getResponseCharset(HttpURLConnection connection)
    {
        if (connection == null) throw new NullArgumentException("connection");

        // Defined in HTTP 1.1 as default content encoding for text types, and we're assuming this
        // is text.
        Charset retval = StandardCharsets.ISO_8859_1;

        String contentType = connection.getContentType();
        if (contentType != null)
        {
            MediaType mediaType = null;
            try
            {
                mediaType = MediaType.parse(contentType);
            }
            catch (IllegalArgumentException e)
            {
                _logger.logWarning("Could not parse content-type header: "
                                   + Strings.javaString(contentType),
                                   e);
            }
            if (mediaType != null)
            {
                retval = mediaType.charset().or(retval);
            }
        }

        return retval;
    }

    protected String getResponse(HttpURLConnection connection) throws HttpException
    {
        if (connection == null) throw new NullArgumentException("connection");

        try
        {
            return CharStreams.toString(new InputStreamReader(connection.getInputStream(),
                                                              getResponseCharset(connection)));
        }
        catch (IOException e)
        {
            throw new HttpException("Error retrieving response.", e);
        }
    }

    protected String getErrorResponse(HttpURLConnection connection) throws HttpException
    {
        if (connection == null) throw new NullArgumentException("connection");

        try
        {
            return CharStreams.toString(new InputStreamReader(connection.getErrorStream(),
                                                              getResponseCharset(connection)));
        }
        catch (IOException e)
        {
            throw new HttpException("Error retrieving error response.", e);
        }
    }

    protected HttpURLConnection openConnection() throws IOException
    {
        return (HttpURLConnection)openConnection(_url);
    }

    protected URLConnection openConnection(URL url) throws IOException
    {
        if (url == null) throw new NullArgumentException("url");
        
        return url.openConnection();
    }

    protected static URL toUrl(URI uri) throws MalformedURLException
    {
        if (uri == null) throw new NullArgumentException("uri");
        if (!uri.isAbsolute())
        {
            throw new IllegalArgumentException("uri is not absolute: " + uri);
        }

        return uri.toURL();
    }

    private final Logger _logger;
    
    private final URL _url;
}
