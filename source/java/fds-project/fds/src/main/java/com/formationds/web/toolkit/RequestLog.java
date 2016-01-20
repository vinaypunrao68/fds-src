/**
 * Copyright (c) 2016 Formation Data Systems.  All rights reserved.
 */
package com.formationds.web.toolkit;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.net.InetSocketAddress;
import java.nio.charset.Charset;
import java.security.Principal;
import java.util.Collection;
import java.util.Enumeration;
import java.util.EventListener;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

import javax.servlet.AsyncContext;
import javax.servlet.DispatcherType;
import javax.servlet.ReadListener;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.ServletInputStream;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;
import javax.servlet.http.HttpUpgradeHandler;
import javax.servlet.http.Part;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.Marker;
import org.apache.logging.log4j.MarkerManager;
import org.eclipse.jetty.http.HttpFields;
import org.eclipse.jetty.http.HttpMethod;
import org.eclipse.jetty.http.HttpURI;
import org.eclipse.jetty.http.HttpVersion;
import org.eclipse.jetty.server.Authentication;
import org.eclipse.jetty.server.HttpChannel;
import org.eclipse.jetty.server.HttpChannelState;
import org.eclipse.jetty.server.HttpInput;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;
import org.eclipse.jetty.server.SessionManager;
import org.eclipse.jetty.server.UserIdentity;
import org.eclipse.jetty.server.UserIdentity.Scope;
import org.eclipse.jetty.server.handler.ContextHandler.Context;
import org.eclipse.jetty.util.Attributes;
import org.eclipse.jetty.util.MultiMap;

import com.formationds.util.Lazy;

/**
 * Provides tools, helpers, and wrappers for HTTP request logging including
 * payload.
 * <p/>
 * Capturing and logging request payload requires wrapping the request's stream
 * and/or reader and funneling bytes read to another stream. In this
 * implementation it is writing the incoming data to a Log4J logger that the
 * request wrapper is initialized with.
 * <p/>
 * In addition to the Logger definition and its level, the logging relies on
 * several log markers for logging of headers and payload. For headers to be
 * logged, the {@link #REQUEST_HEADERS_MARKER} must be set to onMatch="ACCEPT".
 * Likewise for payload logging, the {@link #REQUEST_PAYLOAD_MARKER} must be set
 * to ACCEPT.
 * <p/>
 * In addition, this also tracks open requests when initializing the request
 * wrapper. The {@link #requestComplete(Request)} method must be called upon
 * completion of the request to remove the request from the open tracking map.
 *
 */
// TODO: consider refactoring this to write output to an arbitrary output
// stream/writer
// instead of a logger. The one downside of the writer is that it can block if
// there
// is no one reading the output.
public class RequestLog {

    /**
     * The logger used as the default for all request wrappers
     */
    public static final Logger LOG = LogManager.getLogger( RequestLog.class );

    // NOTE: I originally had these defined with REQUEST_MARKER as the parent
    // and was
    // scratching my head wondering why when I had REQUEST_PAYLOAD_INGEST_STATS
    // = DENY
    // it was still getting logged. It gets logged if any one of the parents is
    // enabled.
    // We could probably invert the hierarchy and make the more specific ones
    // the parent,
    // but i'm going to leave them without parents for now.
    // The Log4J2 doc on markers is pretty sparse and this was not clear to me.
    public static final Marker REQUEST_MARKER = MarkerManager.getMarker( "REQUEST" );
    public static final Marker REQUEST_QUERY_MARKER = MarkerManager.getMarker( "REQUEST_QUERY" );
    public static final Marker REQUEST_HEADERS_MARKER = MarkerManager.getMarker( "REQUEST_HEADERS" );
    public static final Marker REQUEST_PAYLOAD_MARKER = MarkerManager.getMarker( "REQUEST_PAYLOAD" );

    // NOTE: since this is specific to IngestVolumeStats, it might make more
    // sense to define it
    // there. Keeping it together with the others for now.
    public static final Marker REQUEST_PAYLOAD_INGEST_STATS_MARKER = MarkerManager.getMarker( "REQUEST_PAYLOAD_INGEST_STATS" );

    /**
     * Request ID generator.
     *
     * IDs are only unique within a single execution of the JVM process and
     * start over at 0 after a JVM restart.
     */
    private static final AtomicLong REQUEST_ID_GENERATOR = new AtomicLong( 0 );

    /**
     *
     * @return the next available request id
     */
    static long nextRequestId() {
        return REQUEST_ID_GENERATOR.getAndIncrement();
    }

    /**
     * Track open requests for their duration.
     *
     * When a new request wrapper is created, it will first query the open
     * requests and return an existing one if present.
     */
    // Note that This can't rely on the generated ID and has to rely on
    // equals/hashCode of Request
    // unless we come up with a better way to wrap it.
    // TODO: track response too?
    private static final Map<HttpServletRequest, LoggingRequestWrapper<? extends HttpServletRequest>> openRequests = new ConcurrentHashMap<>();

    /**
     * @param request
     */
    static final void requestComplete( HttpServletRequest request ) {
        openRequests.remove( request );
    }

    /**
     * InputStream filter that forwards input to a logger
     */
    static class RequestPayloadLoggingInputStream extends ServletInputStream {

        // NOTE: Request Info, including id, are already in the pre-formatted
        // header so these are not currently used.
        @SuppressWarnings("unused")
        private final HttpServletRequest request;
        @SuppressWarnings("unused")
        private final long requestId;

        private final Logger logger;
        private final Level level;
        private final Marker marker;
        private final String header;

        private final StringBuffer lineBuffer;

        private final ServletInputStream sis;
        private final BufferedInputStream bis;

        public RequestPayloadLoggingInputStream( long requestId, Logger l, Level level, Marker marker, String header,
                                                 HttpServletRequest request ) throws IOException {

            super();

            this.requestId = requestId;
            this.sis = request.getInputStream();
            this.bis = new BufferedInputStream( sis );
            this.logger = l;
            this.level = level;
            this.marker = marker;
            this.header = header;
            this.request = request;

            this.lineBuffer = new StringBuffer( header );

        }

        @Override
        public int readLine( byte[] b, int off, int len ) throws IOException {
            // since we are not actually extending ServletInputStream, need
            // to reimplement this to call our impl of read().
            if ( len <= 0 ) {
                return 0;
            }
            int count = 0, c;

            while ( ( c = read() ) != -1 ) {
                b[off++] = (byte) c;
                count++;
                if ( c == '\n' || count == len ) {
                    break;
                }
            }
            return count > 0 ? count : -1;
        }

        @Override
        public void setReadListener( ReadListener readListener ) {
            sis.setReadListener( readListener );
        }

        public boolean isFinished() {
            return sis.isFinished();
        }

        public boolean isReady() {
            return sis.isReady();
        }

        @Override
        public int read() throws IOException {
            final int r = bis.read();

            if ( r > 0 && logger.isEnabled( level, marker ) ) {
                synchronized ( lineBuffer ) {
                    lineBuffer.append( Character.toChars( r ) );
                    if ( r == '\n' ) {
                        logger.log( level, marker, lineBuffer.toString() );
                        lineBuffer.setLength( header.length() );
                    }
                }
            }
            return r;
        }

        @Override
        public int read( byte[] b, int off, int len ) throws IOException {
            final int r = bis.read( b, off, len );

            if ( r > 0 && logger.isEnabled( level, marker ) ) {
                String body = new String( b, off, r, Charset.forName( "UTF-8" ) );
                synchronized ( lineBuffer ) {
                    traceBody( logger, level, marker, r, body, lineBuffer, header );
                }
            }
            return r;
        }
    }

    /**
     * Log the body payload chunk to the specified logger using the provided
     * line buffer for buffering the output, containing any header information.
     * Any output remaining, representing an incomplete line, is left in the
     * buffer to be logged when additional data is ready.
     *
     * @param logger
     *            the logger to used
     * @param level
     *            the log level to log at
     * @param marker
     *            the log marker, if any
     * @param bytesRead
     *            the number of bytes read in the request
     * @param payloadChunk
     *            the current payload chunk
     * @param buffer
     *            the line buffer possibly containing a header and previously
     *            read data that has not yet been logged
     * @param header
     *            the header (used only for the length to reset the buffer on a
     *            new line)
     *
     * @throws IOException
     */
    static void traceBody( Logger logger,
                           Level level,
                           Marker marker,
                           int bytesRead,
                           String payloadChunk,
                           StringBuffer buffer,
                           String header ) throws IOException {

        if ( !logger.isEnabled( level, marker ) || bytesRead <= 0 )
            return;

        StringReader sr = new StringReader( payloadChunk );
        BufferedReader br = new BufferedReader( sr );
        String line = null;
        int charsRead = 0;
        while ( ( line = br.readLine() ) != null ) {
            charsRead += line.length();
            buffer.append( line );
            logger.log( level, marker, buffer.toString() );
            buffer.setLength( header.length() );
        }

        // Read any remaining character and buffer them but
        // don't flush to the log (we know there is not a complete line)
        if ( charsRead < bytesRead ) {
            char[] toBuf = new char[bytesRead - charsRead];
            int rem = sr.read( toBuf );
            if ( rem > 0 ) {
                charsRead += rem;
                buffer.append( toBuf, 0, rem );
            }
        }
    }

    /**
     *
     */
    // TODO: we only use the InputStream at the moment so this is not fully
    // exercised
    // in our existing code paths
    static class RequestPayloadLoggingReader extends BufferedReader {

        // NOTE: Request Info, including id, are already in the pre-formatted
        // header so they are not currently used.
        @SuppressWarnings("unused")
        private final HttpServletRequest request;
        @SuppressWarnings("unused")
        private final long requestId;
        private final Logger logger;
        private final Level level;
        private final Marker marker;
        private final String header;

        private final StringBuffer lineBuffer;

        RequestPayloadLoggingReader( long requestId, Logger l, Level level, Marker marker, String header,
                                     HttpServletRequest request ) throws IOException {
            this( requestId, l, level, marker, header, request, request.getReader() );
        }

        private RequestPayloadLoggingReader( long requestId, Logger l, Level level, Marker marker, String header,
                                             HttpServletRequest request, BufferedReader reader ) throws IOException {
            super( reader );
            this.requestId = requestId;
            this.request = request;
            this.logger = l;
            this.level = level;
            this.marker = marker;
            this.header = header;

            this.lineBuffer = new StringBuffer( header );
        }

        @Override
        public int read( char[] cbuf, int off, int len ) throws IOException {
            int r = super.read( cbuf, off, len );

            if ( r > 0 && logger.isEnabled( level, marker ) ) {

                String body = new String( cbuf, off, r );
                synchronized ( lineBuffer ) {
                    traceBody( logger, level, marker, r, body, lineBuffer, header );
                }
            }
            return r;
        }
    }

    /**
     * @param requestId
     *            a generated request id
     * @param the
     *            request
     */
    private static String formatRequestLogEntryBase( Long requestId,
                                                     HttpServletRequest request,
                                                     boolean logQueryParams ) {

        // NOTE: destination on any one host is redundant for each message, but
        // if we are ever
        // consolidating logs then it may be important to include (though there
        // will likely be
        // another mechanism of identifying at least the host where the logs
        // originated)
        // remote port is often ephemeral and short lived, so not sure it is
        // useful to include
        StringBuilder sb = new StringBuilder();
        sb.append( "Request [" );
        if ( requestId != null )
            sb.append( requestId ).append( "] [" );
        sb.append( request.getRemoteAddr() ).append( ":" ).append( request.getRemotePort() ).append( " -> " )
        .append( request.getLocalAddr() ).append( ":" ).append( request.getLocalPort() ).append( "] [" )
        .append( request.getMethod() ).append( "  " ).append( request.getRequestURI() ).append( "]" );

        if ( logQueryParams && request.getQueryString() != null )
            sb.append( " Query: [" ).append( filterQueryParameters( request.getQueryString(), "password", "pwd", "p" ) )
            .append( "]" );

        int contentLen = request.getContentLength();
        if ( contentLen != -1 ) {
            sb.append( " [Content-Length=" ).append( request.getContentLength() ).append( "]" );
        }
        return sb.toString();
    }

    /**
     * Filter out query parameters before logging that might, for example,
     * contain passwords or other sensitive information.
     *
     * @param queryString
     * @param params
     *
     * @return the query string with any values matching the list of parameters
     *         masked
     */
    public static String filterQueryParameters( String queryString, String... params ) {

        Objects.requireNonNull( params );

        StringBuffer result = new StringBuffer();

        SortedSet<String> paramSet = new TreeSet<>();
        for ( String p : params ) {
            paramSet.add( p.toLowerCase() );
        }

        String[] tokens = queryString.split( "&" );

        for ( int i = 0; i < tokens.length; i++ ) {
            String t = tokens[i];
            String[] kv = t.split( "=" );

            if ( kv.length != 2 ) {
                // TODO? throw new IllegalStateException("Unexpected query
                // string format");
                result.append( t );
            } else {

                String key = kv[0];
                String val = kv[1];

                if ( paramSet.contains( key.toLowerCase() ) ) {
                    val = "*****";
                }

                result.append( key ).append( "=" ).append( val );
            }
            if ( i + 1 < tokens.length )
                result.append( "&" );

        }

        return result.toString();
    }

    /**
     * Create request logger with default logger of
     * com.formationds.web.toolkit.RequestLog, the generic
     * REQUEST_PAYLOAD_MARKER (all payloads) and logging at a trace level
     *
     * @param r
     * @return the proxy request
     */
    public static final HttpServletRequest newRequestLogger( HttpServletRequest r ) {

        return newRequestLogger( r, REQUEST_PAYLOAD_MARKER );

    }

    /**
     * Create a request logger wrapper with the default logger
     * com.formationds.web.toolkit.RequestLog and the specified payload marker.
     *
     * @param r
     *            the request to wrap
     * @param payloadMarker
     *            to specify the payload marker to log payload under.
     * @return the proxied request
     */
    public static final HttpServletRequest newRequestLogger( HttpServletRequest r, Marker payloadMarker ) {

        return newRequestLogger( r, LogManager.getLogger( RequestLog.class ), Level.TRACE, payloadMarker );

    }

    /**
     * @param r
     * @param logger
     * @param level
     * @return
     */
    @SuppressWarnings("unchecked")
    private static final HttpServletRequest newRequestLogger( HttpServletRequest r,
                                                              Logger logger,
                                                              Level level,
                                                              Marker payloadMarker ) {

        // if it is already wrapped, just return it
        if ( r instanceof LoggingRequestWrapper<?> &&
                ( (LoggingRequestWrapper<?>) r ).getRequest() instanceof HttpServletRequest ) {
            return r;
        }

        LoggingRequestWrapper<? extends HttpServletRequest> w = openRequests.get( r );

        if ( w == null ) {
            w = (LoggingRequestWrapper<HttpServletRequest>) Proxy.newProxyInstance( r.getClass().getClassLoader(),
                                                                                    new Class[] { HttpServletRequest.class,
                                                                                                  LoggingRequestWrapper.class },
                                                                                    new ProxyLoggingRequestWrapper( r,
                                                                                                                    logger,
                                                                                                                    level,
                                                                                                                    payloadMarker ) );

            openRequests.put( r, w );
        }
        return (HttpServletRequest) w;

    }

    /**
     * @param r
     *            the Jetty request
     *
     * @return a wrapped Jetty request
     */
    static final Request newJettyRequestLogger( Request r ) {

        return newJettyRequestLogger( r, LogManager.getLogger( RequestLog.class ), Level.TRACE,
                                      REQUEST_PAYLOAD_MARKER );

    }

    /**
     * @param r
     * @param logger
     * @param level
     *
     * @return the wrapped jetty request
     */
    private static final Request newJettyRequestLogger( Request r, Logger logger, Level level, Marker payloadMarker ) {

        // if it is already wrapped, just return it
        if ( r instanceof LoggingRequestWrapper<?> &&
                ( (LoggingRequestWrapper<?>) r ).getRequest() instanceof Request ) {
            return r;
        }

        LoggingRequestWrapper<? extends HttpServletRequest> w = openRequests.get( r );
        if ( w == null ) {
            w = new LoggingJettyRequestWrapper( r, logger, level, payloadMarker );
            openRequests.put( r, w );
        }
        return (Request) w;
    }

    /**
     * Interface for request wrappers supporting logging.
     */
    public static interface LoggingRequestWrapper<R extends HttpServletRequest> {

        long getRequestId();

        String getRequestInfoHeader();

        R getRequest();

        Logger getLogger();

        Level getLevel();

        Marker getPayloadMarker();

        /**
         * Override the payload log marker used to determine if payload logging
         * is enabled.
         *
         * @param payloadMarker
         */
        void setPayloadMarker( Marker payloadMarker );
    }

    /**
     * Base implementation of the LoggingRequestWrapper
     */
    private static class BaseLoggingRequestWrapper<R extends HttpServletRequest> implements LoggingRequestWrapper<R> {

        private final long requestId = RequestLog.nextRequestId();

        protected final R request;
        protected final Logger logger;
        protected final Level level;
        protected Marker payloadMarker;

        private final Lazy<String> requestInfoHeader;

        /**
         * @param r
         *            the request
         */
        protected BaseLoggingRequestWrapper( R r, Logger logger, Level level, Marker payloadMarker ) {
            this.request = r;
            this.logger = logger;
            this.level = level;
            this.payloadMarker = payloadMarker;

            this.requestInfoHeader = new Lazy<>( () -> formatRequestLogEntryBase( requestId, request,
                                                                                  logger.isEnabled( level,
                                                                                                    REQUEST_QUERY_MARKER ) ) );

            logRequest();
            logHeaders();
        }

        /*
         * (non-Javadoc)
         *
         * @see com.formationds.web.toolkit.LoggingRequestWrapper#getRequestId()
         */
        @Override
        public long getRequestId() {
            return requestId;
        }

        /*
         * (non-Javadoc)
         *
         * @see com.formationds.web.toolkit.LoggingRequestWrapper#
         * getRequestInfoHeader()
         */
        @Override
        public String getRequestInfoHeader() {
            return requestInfoHeader.getInstance();
        }

        void logRequest() {
            if ( logger.isEnabled( level, REQUEST_MARKER ) ) {
                logger.log( level, REQUEST_MARKER, getRequestInfoHeader() );
            }
        }

        void logHeaders() {
            if ( logger.isEnabled( level, REQUEST_HEADERS_MARKER ) ) {
                Enumeration<String> headers = request.getHeaderNames();
                while ( headers.hasMoreElements() ) {
                    String header = headers.nextElement();
                    String value = request.getHeader( header );
                    logger.log( level, REQUEST_HEADERS_MARKER, "{} {} = {}", requestInfoHeader.getInstance(), header,
                                value );
                }
            }
        }

        /*
         * (non-Javadoc)
         *
         * @see com.formationds.web.toolkit.LoggingRequestWrapper#getRequest()
         */
        @Override
        public R getRequest() {
            return request;
        }

        /*
         * (non-Javadoc)
         *
         * @see com.formationds.web.toolkit.LoggingRequestWrapper#getLogger()
         */
        @Override
        public Logger getLogger() {
            return logger;
        }

        /*
         * (non-Javadoc)
         *
         * @see com.formationds.web.toolkit.LoggingRequestWrapper#getLevel()
         */
        @Override
        public Level getLevel() {
            return level;
        }

        /*
         * (non-Javadoc)
         *
         * @see
         * com.formationds.web.toolkit.LoggingRequestWrapper#getPayloadMarker()
         */
        @Override
        public Marker getPayloadMarker() {
            return payloadMarker;
        }

        protected BufferedReader getLoggingRequestReader() throws IOException {
            return new RequestPayloadLoggingReader( getRequestId(), logger, level, payloadMarker,
                                                    getRequestInfoHeader(), request );
        }

        protected ServletInputStream getLoggingRequestInputStream() throws IOException {
            return new RequestPayloadLoggingInputStream( getRequestId(), logger, level, payloadMarker,
                                                         getRequestInfoHeader(), request );
        }

        @Override
        public void setPayloadMarker( Marker payloadMarker ) {
            this.payloadMarker = payloadMarker;
        }
    }

    /**
     * LoggingRequestWrapper of an HttpServletRequest via a dynamic proxy
     */
    private static class ProxyLoggingRequestWrapper extends BaseLoggingRequestWrapper<HttpServletRequest>
    implements InvocationHandler {

        /**
         * @param r
         *            the request
         */
        private ProxyLoggingRequestWrapper( HttpServletRequest r, Logger logger, Level level, Marker marker ) {
            super( r, logger, level, marker );
        }

        @Override
        public Object invoke( Object proxy, Method method, Object[] args ) throws Throwable {
            switch ( method.getName() ) {
                case "getReader":
                    return super.getLoggingRequestReader();

                case "getInputStream":
                    return super.getLoggingRequestInputStream();

                case "hashCode":
                    return request.hashCode();

                case "equals":
                    if ( args.length == 1 ) {
                        Object that = args[0];
                        if ( this == that )
                            return true;
                        if ( !( that instanceof LoggingRequestWrapper<?> ) )
                            return false;
                        LoggingRequestWrapper<?> w = (LoggingRequestWrapper<?>) that;
                        return getRequest().equals( w.getRequest() );
                    }
                    // Otherwise fall-through to default case

                default:
                    return method.invoke( request, args );
            }
        }
    }

    /**
     * A Jetty Request implementation of the LoggingRequestWrapper. All request
     * operations other than the Stream/Reader operations are delegated directly
     * to the wrapped request. Stream/Reader operations are delegated to a
     * LoggingRequestWrapper.
     */
    private static class LoggingJettyRequestWrapper extends Request implements LoggingRequestWrapper<Request> {

        private final BaseLoggingRequestWrapper<Request> loggingRequestWrapper;
        private final Request delegate;

        /**
         * @param r
         *            the request
         */
        private LoggingJettyRequestWrapper( Request r, Logger logger, Level level, Marker marker ) {
            // we extend Request, but everything that can be delegated is
            // delegated
            super( r.getHttpChannel(), r.getHttpInput() );
            this.delegate = r;
            loggingRequestWrapper = new BaseLoggingRequestWrapper<>( delegate, logger, level, marker );
        }

        @Override
        public ServletInputStream getInputStream() throws IOException {
            return loggingRequestWrapper.getLoggingRequestInputStream();
        }

        @Override
        public BufferedReader getReader() throws IOException {
            return loggingRequestWrapper.getLoggingRequestReader();
        }

        public long getRequestId() {
            return loggingRequestWrapper.getRequestId();
        }

        public String getRequestInfoHeader() {
            return loggingRequestWrapper.getRequestInfoHeader();
        }

        public Request getRequest() {
            return loggingRequestWrapper.getRequest();
        }

        public Logger getLogger() {
            return loggingRequestWrapper.getLogger();
        }

        public Level getLevel() {
            return loggingRequestWrapper.getLevel();
        }

        public Marker getPayloadMarker() {
            return loggingRequestWrapper.getPayloadMarker();
        }

        public void setPayloadMarker( Marker payloadMarker ) {
            loggingRequestWrapper.setPayloadMarker( payloadMarker );
        }

        public int hashCode() {
            return delegate.hashCode();
        }

        public boolean equals( Object obj ) {
            return delegate.equals( obj );
        }

        public HttpFields getHttpFields() {
            return delegate.getHttpFields();
        }

        public HttpInput<?> getHttpInput() {
            return delegate.getHttpInput();
        }

        public void addEventListener( EventListener listener ) {
            delegate.addEventListener( listener );
        }

        public void extractParameters() {
            delegate.extractParameters();
        }

        public void extractFormParameters( MultiMap<String> params ) {
            delegate.extractFormParameters( params );
        }

        public AsyncContext getAsyncContext() {
            return delegate.getAsyncContext();
        }

        public HttpChannelState getHttpChannelState() {
            return delegate.getHttpChannelState();
        }

        public Object getAttribute( String name ) {
            return delegate.getAttribute( name );
        }

        public Enumeration<String> getAttributeNames() {
            return delegate.getAttributeNames();
        }

        public Attributes getAttributes() {
            return delegate.getAttributes();
        }

        public Authentication getAuthentication() {
            return delegate.getAuthentication();
        }

        public String getAuthType() {
            return delegate.getAuthType();
        }

        public String getCharacterEncoding() {
            return delegate.getCharacterEncoding();
        }

        public HttpChannel<?> getHttpChannel() {
            return delegate.getHttpChannel();
        }

        public int getContentLength() {
            return delegate.getContentLength();
        }

        public long getContentLengthLong() {
            return delegate.getContentLengthLong();
        }

        public long getContentRead() {
            return delegate.getContentRead();
        }

        public String getContentType() {
            return delegate.getContentType();
        }

        public Context getContext() {
            return delegate.getContext();
        }

        public String getContextPath() {
            return delegate.getContextPath();
        }

        public Cookie[] getCookies() {
            return delegate.getCookies();
        }

        public long getDateHeader( String name ) {
            return delegate.getDateHeader( name );
        }

        public DispatcherType getDispatcherType() {
            return delegate.getDispatcherType();
        }

        public String getHeader( String name ) {
            return delegate.getHeader( name );
        }

        public Enumeration<String> getHeaderNames() {
            return delegate.getHeaderNames();
        }

        public Enumeration<String> getHeaders( String name ) {
            return delegate.getHeaders( name );
        }

        public int getInputState() {
            return delegate.getInputState();
        }

        public int getIntHeader( String name ) {
            return delegate.getIntHeader( name );
        }

        public Locale getLocale() {
            return delegate.getLocale();
        }

        public Enumeration<Locale> getLocales() {
            return delegate.getLocales();
        }

        public String getLocalAddr() {
            return delegate.getLocalAddr();
        }

        public String getLocalName() {
            return delegate.getLocalName();
        }

        public int getLocalPort() {
            return delegate.getLocalPort();
        }

        public String getMethod() {
            return delegate.getMethod();
        }

        public String getParameter( String name ) {
            return delegate.getParameter( name );
        }

        public Map<String, String[]> getParameterMap() {
            return delegate.getParameterMap();
        }

        public Enumeration<String> getParameterNames() {
            return delegate.getParameterNames();
        }

        public String[] getParameterValues( String name ) {
            return delegate.getParameterValues( name );
        }

        public MultiMap<String> getQueryParameters() {
            return delegate.getQueryParameters();
        }

        public void setQueryParameters( MultiMap<String> queryParameters ) {
            delegate.setQueryParameters( queryParameters );
        }

        public void setContentParameters( MultiMap<String> contentParameters ) {
            delegate.setContentParameters( contentParameters );
        }

        public void resetParameters() {
            delegate.resetParameters();
        }

        public String getPathInfo() {
            return delegate.getPathInfo();
        }

        public String getPathTranslated() {
            return delegate.getPathTranslated();
        }

        public String getProtocol() {
            return delegate.getProtocol();
        }

        public HttpVersion getHttpVersion() {
            return delegate.getHttpVersion();
        }

        public String getQueryEncoding() {
            return delegate.getQueryEncoding();
        }

        public String getQueryString() {
            return delegate.getQueryString();
        }

        public String getRealPath( String path ) {
            return delegate.getRealPath( path );
        }

        public InetSocketAddress getRemoteInetSocketAddress() {
            return delegate.getRemoteInetSocketAddress();
        }

        public String getRemoteAddr() {
            return delegate.getRemoteAddr();
        }

        public String getRemoteHost() {
            return delegate.getRemoteHost();
        }

        public int getRemotePort() {
            return delegate.getRemotePort();
        }

        public String getRemoteUser() {
            return delegate.getRemoteUser();
        }

        public RequestDispatcher getRequestDispatcher( String path ) {
            return delegate.getRequestDispatcher( path );
        }

        public String getRequestedSessionId() {
            return delegate.getRequestedSessionId();
        }

        public String getRequestURI() {
            return delegate.getRequestURI();
        }

        public StringBuffer getRequestURL() {
            return delegate.getRequestURL();
        }

        public Response getResponse() {
            return delegate.getResponse();
        }

        public StringBuilder getRootURL() {
            return delegate.getRootURL();
        }

        public String getScheme() {
            return delegate.getScheme();
        }

        public String getServerName() {
            return delegate.getServerName();
        }

        public int getServerPort() {
            return delegate.getServerPort();
        }

        public ServletContext getServletContext() {
            return delegate.getServletContext();
        }

        public String getServletName() {
            return delegate.getServletName();
        }

        public String getServletPath() {
            return delegate.getServletPath();
        }

        public ServletResponse getServletResponse() {
            return delegate.getServletResponse();
        }

        public String changeSessionId() {
            return delegate.changeSessionId();
        }

        public HttpSession getSession() {
            return delegate.getSession();
        }

        public HttpSession getSession( boolean create ) {
            return delegate.getSession( create );
        }

        public SessionManager getSessionManager() {
            return delegate.getSessionManager();
        }

        public long getTimeStamp() {
            return delegate.getTimeStamp();
        }

        public HttpURI getUri() {
            return delegate.getUri();
        }

        public UserIdentity getUserIdentity() {
            return delegate.getUserIdentity();
        }

        public UserIdentity getResolvedUserIdentity() {
            return delegate.getResolvedUserIdentity();
        }

        public Scope getUserIdentityScope() {
            return delegate.getUserIdentityScope();
        }

        public Principal getUserPrincipal() {
            return delegate.getUserPrincipal();
        }

        public boolean isHandled() {
            return delegate.isHandled();
        }

        public boolean isAsyncStarted() {
            return delegate.isAsyncStarted();
        }

        public boolean isAsyncSupported() {
            return delegate.isAsyncSupported();
        }

        public boolean isRequestedSessionIdFromCookie() {
            return delegate.isRequestedSessionIdFromCookie();
        }

        public boolean isRequestedSessionIdFromUrl() {
            return delegate.isRequestedSessionIdFromUrl();
        }

        public boolean isRequestedSessionIdFromURL() {
            return delegate.isRequestedSessionIdFromURL();
        }

        public boolean isRequestedSessionIdValid() {
            return delegate.isRequestedSessionIdValid();
        }

        public boolean isSecure() {
            return delegate.isSecure();
        }

        public void setSecure( boolean secure ) {
            delegate.setSecure( secure );
        }

        public boolean isUserInRole( String role ) {
            return delegate.isUserInRole( role );
        }

        public HttpSession recoverNewSession( Object key ) {
            return delegate.recoverNewSession( key );
        }

        public void removeAttribute( String name ) {
            delegate.removeAttribute( name );
        }

        public void removeEventListener( EventListener listener ) {
            delegate.removeEventListener( listener );
        }

        public void saveNewSession( Object key, HttpSession session ) {
            delegate.saveNewSession( key, session );
        }

        public void setAsyncSupported( boolean supported ) {
            delegate.setAsyncSupported( supported );
        }

        public void setAttribute( String name, Object value ) {
            delegate.setAttribute( name, value );
        }

        public void setAttributes( Attributes attributes ) {
            delegate.setAttributes( attributes );
        }

        public void setAuthentication( Authentication authentication ) {
            delegate.setAuthentication( authentication );
        }

        public void setCharacterEncoding( String encoding ) throws UnsupportedEncodingException {
            delegate.setCharacterEncoding( encoding );
        }

        public void setCharacterEncodingUnchecked( String encoding ) {
            delegate.setCharacterEncodingUnchecked( encoding );
        }

        public void setContentType( String contentType ) {
            delegate.setContentType( contentType );
        }

        public void setContext( Context context ) {
            delegate.setContext( context );
        }

        public boolean takeNewContext() {
            return delegate.takeNewContext();
        }

        public void setContextPath( String contextPath ) {
            delegate.setContextPath( contextPath );
        }

        public void setCookies( Cookie[] cookies ) {
            delegate.setCookies( cookies );
        }

        public void setDispatcherType( DispatcherType type ) {
            delegate.setDispatcherType( type );
        }

        public void setHandled( boolean h ) {
            delegate.setHandled( h );
        }

        public void setMethod( HttpMethod httpMethod, String method ) {
            delegate.setMethod( httpMethod, method );
        }

        public boolean isHead() {
            return delegate.isHead();
        }

        public void setPathInfo( String pathInfo ) {
            delegate.setPathInfo( pathInfo );
        }

        public void setHttpVersion( HttpVersion version ) {
            delegate.setHttpVersion( version );
        }

        public void setQueryEncoding( String queryEncoding ) {
            delegate.setQueryEncoding( queryEncoding );
        }

        public void setQueryString( String queryString ) {
            delegate.setQueryString( queryString );
        }

        public void setRemoteAddr( InetSocketAddress addr ) {
            delegate.setRemoteAddr( addr );
        }

        public void setRequestedSessionId( String requestedSessionId ) {
            delegate.setRequestedSessionId( requestedSessionId );
        }

        public void setRequestedSessionIdFromCookie( boolean requestedSessionIdCookie ) {
            delegate.setRequestedSessionIdFromCookie( requestedSessionIdCookie );
        }

        public void setRequestURI( String requestURI ) {
            delegate.setRequestURI( requestURI );
        }

        public void setScheme( String scheme ) {
            delegate.setScheme( scheme );
        }

        public void setServerName( String host ) {
            delegate.setServerName( host );
        }

        public void setServerPort( int port ) {
            delegate.setServerPort( port );
        }

        public void setServletPath( String servletPath ) {
            delegate.setServletPath( servletPath );
        }

        public void setSession( HttpSession session ) {
            delegate.setSession( session );
        }

        public void setSessionManager( SessionManager sessionManager ) {
            delegate.setSessionManager( sessionManager );
        }

        public void setTimeStamp( long ts ) {
            delegate.setTimeStamp( ts );
        }

        public void setUri( HttpURI uri ) {
            delegate.setUri( uri );
        }

        public void setUserIdentityScope( Scope scope ) {
            delegate.setUserIdentityScope( scope );
        }

        public AsyncContext startAsync() throws IllegalStateException {
            return delegate.startAsync();
        }

        public AsyncContext startAsync( ServletRequest servletRequest,
                                        ServletResponse servletResponse ) throws IllegalStateException {
            return delegate.startAsync( servletRequest, servletResponse );
        }

        public String toString() {
            return delegate.toString();
        }

        public boolean authenticate( HttpServletResponse response ) throws IOException, ServletException {
            return delegate.authenticate( response );
        }

        public Part getPart( String name ) throws IOException, ServletException {
            return delegate.getPart( name );
        }

        public Collection<Part> getParts() throws IOException, ServletException {
            return delegate.getParts();
        }

        public void login( String username, String password ) throws ServletException {
            delegate.login( username, password );
        }

        public void logout() throws ServletException {
            delegate.logout();
        }

        public void mergeQueryParameters( String newQuery, boolean updateQueryString ) {
            delegate.mergeQueryParameters( newQuery, updateQueryString );
        }

        public <T extends HttpUpgradeHandler> T upgrade( Class<T> handlerClass ) throws IOException, ServletException {
            return delegate.upgrade( handlerClass );
        }
    }

}
