package com.formationds.spike.later;

import io.undertow.io.IoCallback;
import io.undertow.io.Sender;
import io.undertow.server.HttpServerExchange;
import io.undertow.server.handlers.Cookie;
import io.undertow.util.HeaderValues;
import io.undertow.util.HttpString;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.Deque;
import java.util.Map;
import java.util.stream.Collectors;

public class HttpContext {
    private HttpServerExchange exchange;
    private Map<String, String> routeParameters;

    private static final HttpString CONTENT_TYPE = new HttpString("Content-Type");
    private Sender sender;

    public HttpContext(HttpServerExchange exchange, Map<String, String> routeParameters) {
        this.exchange = exchange;
        this.routeParameters = routeParameters;
    }

    public HttpServerExchange getExchange() {
        return exchange;
    }

    public Map<String, String> getRouteParameters() {
        return routeParameters;
    }

    public void setResponseContentType(String contentType) {
        exchange.getResponseHeaders().add(CONTENT_TYPE, contentType);
    }

    public void setResponseStatus(int status) {
        exchange.setResponseCode(status);
    }

    public String getRequestHeader(String headerName) {
        HeaderValues values = exchange.getRequestHeaders().get(headerName);
        if (values == null || values.size() == 0) {
            return null;
        }
        return values.get(0);
    }

    public String getRequestMethod() {
        return exchange.getRequestMethod().toString();
    }

    public String getRequestURI() {
        return exchange.getRequestURI();
    }

    public Collection<String> getRequestHeaderNames() {
        return exchange.getRequestHeaders().getHeaderNames()
                .stream()
                .map(hs -> hs.toString())
                .collect(Collectors.toList());
    }

    public Collection<String> getRequestHeaderValues(String header) {
        return exchange.getRequestHeaders().get(header);
    }

    public Map<String, Deque<String>> getQueryString() {
        return exchange.getQueryParameters();
    }


    public String getRequestContentType() {
        return exchange.getRequestHeaders().getFirst(CONTENT_TYPE);
    }

    public void addResponseHeader(String key, String value) {
        exchange.getResponseHeaders().add(new HttpString(key), value);
    }

    public String getRouteParameter(String routeParameter) {
        if (!routeParameters.containsKey(routeParameter)) {
            throw new IllegalArgumentException("Missing route parameter '" + routeParameter + "'");
        }

        return routeParameters.get(routeParameter);
    }

    public void addCookie(String name, Cookie cookie) {
        exchange.getResponseCookies().put(name, cookie);
    }

    public OutputStream getOutputStream() {
        return new ExchangeOutputStream(exchange);
    }

    public InputStream getInputStream() {
        exchange.startBlocking();
        return exchange.getInputStream();
    }


    class ExchangeOutputStream extends OutputStream {
        private HttpServerExchange httpExchange;
        private final IoCallback cb;

        public ExchangeOutputStream(HttpServerExchange httpExchange) {
            this.httpExchange = httpExchange;
            cb = new IoCallback() {
                @Override
                public void onComplete(HttpServerExchange exchange, Sender sender) {

                }

                @Override
                public void onException(HttpServerExchange exchange, Sender sender, IOException exception) {

                }
            };
        }

        @Override
        public void write(byte[] b) throws IOException {
            httpExchange.getResponseSender().send(ByteBuffer.wrap(b), cb);
        }

        @Override
        public void write(byte[] b, int off, int len) throws IOException {
            httpExchange.getResponseSender().send(ByteBuffer.wrap(b, off, len), cb);
        }

        @Override
        public void write(int b) throws IOException {
            byte[] buf = new byte[1];
            buf[0] = (byte) b;
            write(buf);
        }

        @Override
        public void close() throws IOException {
            httpExchange.getResponseSender().close(cb);
        }
    }
}
