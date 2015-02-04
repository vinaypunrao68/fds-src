package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.ProtocolException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.AccessDeniedException;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Strings;
import com.formationds.commons.util.Uris;
import com.formationds.commons.util.functional.ExceptionThrowingConsumer;
import com.formationds.commons.util.functional.ExceptionThrowingFunction;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.AbstractHttpOperation;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.google.common.io.CharStreams;
import com.google.common.net.MediaType;

// @eclipseFormat:off
public abstract class AbstractHttpEndpoint<
    ThisT extends AbstractHttpEndpoint<ThisT, OperationT>,
    OperationT extends AbstractHttpOperation<OperationT, ThisT>>
extends Endpoint<ThisT, OperationT>
// @eclipseFormat:on
{
    public AbstractHttpEndpoint(URI uri, Logger logger) throws MalformedURLException
    {
        this(toUrl(uri), logger);
    }

    public AbstractHttpEndpoint(URL url, Logger logger)
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
    // @eclipseFormat:off
    public void doVisit(OperationT operation,
                        WorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        operation.accept(getThis(), listener);
    }

    // @eclipseFormat:off
    public void visit(OperationT operation,
                      WorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        HttpURLConnection connection;
        try
        {
            URI relativeUri = operation.getRelativeUri();
            if (relativeUri == null)
            {
                connection = openConnection();
            }
            else
            {
                connection = openRelativeConnection(relativeUri);
            }
        }
        catch (IOException e)
        {
            throw new ExecutionException(e);
        }

        try
        {
            operation.exec(getThis(), connection, listener);
        }
        finally
        {
            connection.disconnect();
        }
    }

    // @eclipseFormat:off
    public void doPut(HttpURLConnection connection, String content, Charset charset)
            throws HttpException
    // @eclipseFormat:on
    {
        if (connection == null) throw new NullArgumentException("connection");
        if (content == null) throw new NullArgumentException("content");
        if (charset == null) throw new NullArgumentException("charset");

        try
        {
            connection.setRequestMethod("PUT");
        }
        catch (ProtocolException e)
        {
            // Should be impossible.
            throw new RuntimeException("Unexpected error setting request method to PUT.");
        }
        connection.setDoInput(true);
        connection.setDoOutput(true);

        putRequest(connection, content, charset);

        handleResponse(connection, c -> null);
    }

    public String doGet(HttpURLConnection connection) throws HttpException
    {
        if (connection == null) throw new NullArgumentException("connection");

        try
        {
            connection.setRequestMethod("GET");
        }
        catch (ProtocolException e)
        {
            // Should be impossible.
            throw new RuntimeException("Unexpected error setting request method to GET.", e);
        }
        connection.setDoInput(true);
        connection.setDoOutput(false);

        return handleResponse(connection, c -> getResponse(c));
    }

    // @eclipseFormat:off
    protected void handleResponseNoReturn(
        HttpURLConnection connection,
        ExceptionThrowingConsumer<HttpURLConnection, HttpException> consumer) throws HttpException
    // @eclipseFormat:on
    {
        handleResponse(connection, (c ->
        {
            consumer.accept(c);
            return null;
        }));
    }

    // @eclipseFormat:off
    protected <T> T handleResponse(
        HttpURLConnection connection,
        ExceptionThrowingFunction<HttpURLConnection, T, HttpException> func) throws HttpException
    // @eclipseFormat:on
    {
        int responseCode = getResponseCode(connection);
        int responseClass = responseCode / 100;

        switch (responseClass)
        {
        case 2:
            return func.apply(connection);
        case 4:
            throw new HttpClientException(responseCode,
                                          getResponseMessage(connection),
                                          getErrorResponse(connection));
        case 5:
            throw new HttpServerException(responseCode,
                                          getResponseMessage(connection),
                                          getErrorResponse(connection));
        default:
            throw new HttpException(responseCode, getResponseMessage(connection));
        }
    }

    protected String getResponseMessage(HttpURLConnection connection) throws HttpException
    {
        if (connection == null) throw new NullArgumentException("connection");

        try
        {
            return connection.getResponseMessage();
        }
        catch (IOException e)
        {
            throw new HttpException("Error retrieving response message.", e);
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

    protected class CopyHelper extends Endpoint<ThisT, OperationT>.CopyHelper
    {
        public final Logger logger = _logger;
        public final URL url = _url;
    }

    protected AbstractHttpEndpoint(CopyHelper helper)
    {
        super(helper);

        _logger = helper.logger;
        _url = helper.url;
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

        Charset responseCharset = getResponseCharset(connection);
        try (InputStreamReader input =
                new InputStreamReader(connection.getInputStream(), responseCharset))
        {
            return CharStreams.toString(input);
        }
        catch (IOException e)
        {
            throw new HttpException("Error retrieving response.", e);
        }
    }

    protected int getResponseCode(HttpURLConnection connection) throws HttpException
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
        return responseCode;
    }

    @SuppressWarnings("unchecked")
    protected final ThisT getThis()
    {
        return (ThisT)this;
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

    protected HttpURLConnection openRelativeConnection(URI relativeUri) throws IOException
    {
        if (relativeUri == null) throw new NullArgumentException("relativeUri");

        URI base;
        try
        {
            base = _url.toURI();
        }
        catch (URISyntaxException e)
        {
            // Shouldn't be possible.
            throw new RuntimeException("Unexpected error converting URL to URI.", e);
        }
        URI resolved = Uris.resolve(base, relativeUri);

        if (!Uris.isDescendant(base, resolved))
        {
            throw new AccessDeniedException("Access to " + relativeUri.toString()
                                            + ", which is not a descendant of " + base.toString()
                                            + " is denied.");
        }

        try
        {
            return (HttpURLConnection)openConnection(resolved.toURL());
        }
        catch (MalformedURLException e)
        {
            // These should both be impossible since we don't give access to the full URI.
            throw new RuntimeException("Unexpected error opening connection.", e);
        }
        catch (IOException e)
        {
            throw e;
        }
    }

    // @eclipseFormat:off
    protected void putRequest(HttpURLConnection connection, String content, Charset charset)
            throws HttpException
    // @eclipseFormat:on
    {
        if (connection == null) throw new NullArgumentException("connection");
        if (content == null) throw new NullArgumentException("content");
        if (charset == null) throw new NullArgumentException("charset");

        try (OutputStreamWriter output =
                new OutputStreamWriter(connection.getOutputStream()))
        {
            output.write(content);
        }
        catch (IOException e)
        {
            throw new HttpException("Error sending request.", e);
        }
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
