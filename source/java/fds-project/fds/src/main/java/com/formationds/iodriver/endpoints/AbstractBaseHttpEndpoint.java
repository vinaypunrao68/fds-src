package com.formationds.iodriver.endpoints;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.ProtocolException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.AccessDeniedException;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Strings;
import com.formationds.commons.util.Uris;
import com.formationds.commons.util.functional.ExceptionThrowingFunction;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.operations.BaseHttpOperation;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
import com.google.common.io.CharStreams;
import com.google.common.net.MediaType;

/**
 * Basic HTTP endpoint.
 */
// @eclipseFormat:off
public abstract class AbstractBaseHttpEndpoint<
        ThisT extends AbstractBaseHttpEndpoint<ThisT, ConnectionT>,
        ConnectionT extends HttpURLConnection> implements BaseHttpEndpoint<ConnectionT>
// @eclipseFormat:on
{
    /**
     * Constructor.
     * 
     * @param uri The base URI for this endpoint. Must be a valid absolute URL.
     * @param logger The logger to log to.
     *
     * @throws MalformedURLException when {@code uri} is a malformed URL.
     */
    public AbstractBaseHttpEndpoint(URI uri, Logger logger, Class<ConnectionT> connectionType)
            throws MalformedURLException
    {
        this(toUrl(uri), logger, connectionType);
    }

    /**
     * Constructor.
     * 
     * @param url The base URL for this endpoint.
     * @param logger The logger to log to.
     */
    public AbstractBaseHttpEndpoint(URL url, Logger logger, Class<ConnectionT> connectionType)
    {
        if (url == null) throw new NullArgumentException("url");
        if (logger == null) throw new NullArgumentException("logger");
        if (connectionType == null) throw new NullArgumentException("connectionType");
        
        {
            String scheme = url.getProtocol();
            if (scheme == null
                || !(scheme.equalsIgnoreCase("http") || scheme.equalsIgnoreCase("https")))
            {
                throw new IllegalArgumentException("url is not HTTP or HTTPS: " + url);
            }
        }

        _connectionType = connectionType;
        _logger = logger;
        _url = url;
    }

    @Override
    public void doWrite(ConnectionT connection,
                        String content,
                        Charset charset) throws HttpException
    {
        if (connection == null) throw new NullArgumentException("connection");
        if (content == null) throw new NullArgumentException("content");
        if (charset == null) throw new NullArgumentException("charset");

        connection.setDoInput(true);
        connection.setDoOutput(true);

        writeToRequest(connection, content, charset);

        handleResponse(connection, c -> null);
    }

    @Override
    public String doWriteThenRead(ConnectionT connection,
                                  String content,
                                  Charset charset) throws HttpException
    {
        if (connection == null) throw new NullArgumentException("connection");
        if (content == null) throw new NullArgumentException("content");
        if (charset == null) throw new NullArgumentException("charset");

        connection.setDoInput(true);
        connection.setDoOutput(true);

        writeToRequest(connection, content, charset);

        return handleResponse(connection, c -> getResponse(c));
    }

    @Override
    public String doRead(ConnectionT connection) throws HttpException
    {
        if (connection == null) throw new NullArgumentException("connection");

        // FIXME: These can only be called on a not-yet-open connection, should be done before
        //        returning the connection to the operation. See also doWrite().
        connection.setDoInput(true);
        connection.setDoOutput(false);

        return handleResponse(connection, c -> getResponse(c));
    }

    @Override
    public Class<ConnectionT> getConnectionType()
    {
        return _connectionType;
    }
    
    /**
     * Get the base URL of this endpoint.
     * 
     * @return An absolute URL.
     */
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

    @Override
    public void visit(BaseHttpOperation<ConnectionT> operation,
                      AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        ConnectionT connection;
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
            String requestMethod = operation.getRequestMethod();
            try
            {
                connection.setRequestMethod(requestMethod);
            }
            catch (ProtocolException e)
            {
                throw new ExecutionException(
                        "Error setting request method to " + javaString(requestMethod) + ".", e);
            }

            typedVisit(operation, connection, listener);
        }
        finally
        {
            connection.disconnect();
        }
    }
    
    @Override
    public void visit(Operation operation,
                      AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");
        
        BaseHttpOperation<ConnectionT> typedOperation = null;
        if (operation instanceof BaseHttpOperation)
        {
            BaseHttpOperation<?> partiallyTypedOperation = (BaseHttpOperation<?>)operation;
            if (partiallyTypedOperation.getConnectionType().isAssignableFrom(getConnectionType()))
            {
                @SuppressWarnings("unchecked")
                BaseHttpOperation<ConnectionT> checkedTypedOperation =
                        (BaseHttpOperation<ConnectionT>)partiallyTypedOperation;
                typedOperation = checkedTypedOperation;
            }
        }
        
        if (typedOperation != null)
        {
            visit(typedOperation, listener);
        }
        else
        {
            operation.accept(this, listener);
        }
    }
    
    /**
     * Extend this class to allow deep copies even when superclass private members aren't available.
     */
    protected class CopyHelper
    {
        public final Class<ConnectionT> connectionType = _connectionType;
        
        /**
         * A reference to the source object's logger. Shouldn't need copying.
         */
        public final Logger logger = _logger;

        /**
         * A reference to the source object's base URL. Shouldn't need copying.
         */
        public final URL url = _url;
    }

    /**
     * Copy constructor.
     * 
     * @param helper Object holding copied values to assign to the new object.
     */
    protected AbstractBaseHttpEndpoint(CopyHelper helper)
    {
        _connectionType = helper.connectionType;
        _logger = helper.logger;
        _url = helper.url;
    }

    /**
     * Get the error response from an open connection.
     * 
     * @param connection Get the error response here.
     * 
     * @return The body of the error response.
     * 
     * @throws HttpException when there was an error retrieving the error.
     */
    protected String getErrorResponse(ConnectionT connection) throws HttpException
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

    /**
     * Get the response body from an open connection.
     * 
     * @param connection Get the body here.
     * 
     * @return The response body text.
     * 
     * @throws HttpException when there is an error reading the response.
     */
    protected String getResponse(ConnectionT connection) throws HttpException
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

    /**
     * Get the charset of a response from an open connection.
     * 
     * @param connection Get the charset here.
     * 
     * @return The response charset if it could be determined, or ISO-8859-1 (HTTP default).
     */
    protected Charset getResponseCharset(ConnectionT connection)
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

    /**
     * Get the response code from an open connection.
     * 
     * @param connection Get the code from here.
     * 
     * @return The response code.
     * 
     * @throws HttpException when an error occurs getting the response code or the response code
     *             does not appear valid (>= 0).
     */
    protected int getResponseCode(ConnectionT connection) throws HttpException
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
            throw new HttpException(responseCode,
                                    connection.getURL().toString(),
                                    connection.getRequestMethod());
        }
        return responseCode;
    }

    /**
     * Get the response text (i.e. OK, NOT FOUND, etc.).
     * 
     * @param connection Get the response text from here.
     * 
     * @return The response text.
     * 
     * @throws HttpException when an error occurs getting the response message.
     */
    protected String getResponseMessage(ConnectionT connection) throws HttpException
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

    @SuppressWarnings("unchecked")
    protected ThisT getThis()
    {
        return (ThisT)this;
    }
    
    /**
     * Process the response code from a request, move to error handling if response code indicates
     * error.
     * 
     * @param connection Get the response from here.
     * @param func Pass the connection on to this if the response indicates success.
     * 
     * @return The output of {@code func}.
     * 
     * @throws HttpException when an error occurs sending the request or receiving the response.
     */
    // @eclipseFormat:off
    protected <T> T handleResponse(
        ConnectionT connection,
        ExceptionThrowingFunction<ConnectionT, T, HttpException> func) throws HttpException
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
                                          connection.getURL().toString(),
                                          connection.getRequestMethod(),
                                          getResponseMessage(connection),
                                          getErrorResponse(connection));
        case 5:
            throw new HttpServerException(responseCode,
                                          connection.getURL().toString(),
                                          connection.getRequestMethod(),
                                          getResponseMessage(connection),
                                          getErrorResponse(connection));
        default:
            throw new HttpException(responseCode,
                                    connection.getURL().toString(),
                                    connection.getRequestMethod(),
                                    getResponseMessage(connection));
        }
    }

    /**
     * Open a connection to the base URL.
     * 
     * @return A connection.
     * 
     * @throws IOException when there is an error connecting.
     */
    protected ConnectionT openConnection() throws IOException
    {
        return openConnection(_url);
    }

    /**
     * Open a connection to an absolute URL.
     * 
     * @param url The URL.
     * 
     * @return A connection.
     * 
     * @throws IOException when there is an error connecting.
     */
    protected ConnectionT openConnection(URL url) throws IOException
    {
        if (url == null) throw new NullArgumentException("url");
        
        @SuppressWarnings("unchecked")
        ConnectionT typedConnection = (ConnectionT)url.openConnection();
        
        return typedConnection;
    }

    /**
     * Open a connection to a relative URL.
     * 
     * @param relativeUri The URL to resolve against this endpoint's base.
     * 
     * @return A connection.
     * 
     * @throws IOException when there is an error connecting.
     */
    protected ConnectionT openRelativeConnection(URI relativeUri) throws IOException
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
            return openConnection(resolved.toURL());
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

    protected abstract void typedVisit(BaseHttpOperation<ConnectionT> operation,
                                       ConnectionT connection,
                                       AbstractWorkloadEventListener listener)
            throws ExecutionException;

    /**
     * Write a request body to a connection. Connection must be valid for writing, e.g. POST.
     * 
     * @param connection The connection to write to.
     * @param content The content to write.
     * @param charset The charset of {@code content}.
     * 
     * @throws HttpException when an error occurs sending the content.
     */
    // @eclipseFormat:off
    protected void writeToRequest(ConnectionT connection, String content, Charset charset)
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

    /**
     * Convert a URI to a URL.
     * 
     * @param uri The absolute URI to convert.
     * 
     * @return A URL.
     * 
     * @throws MalformedURLException when {@code uri} is not a valid absolute URL.
     */
    protected static URL toUrl(URI uri) throws MalformedURLException
    {
        if (uri == null) throw new NullArgumentException("uri");
        if (!uri.isAbsolute())
        {
            throw new IllegalArgumentException("uri is not absolute: " + uri);
        }

        return uri.toURL();
    }

    /**
     * Type of connection offered by this endpoint.
     */
    private final Class<ConnectionT> _connectionType;
    
    /**
     * Logs.
     */
    private final Logger _logger;

    /**
     * Base URL for connections.
     */
    private final URL _url;
}
