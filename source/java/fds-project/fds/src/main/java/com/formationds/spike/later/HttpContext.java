package com.formationds.spike.later;

import com.google.common.collect.Lists;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;
import org.eclipse.jetty.util.MultiMap;

import javax.servlet.http.Cookie;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.stream.Collectors;

// TODO: refactor this into an interface
public class HttpContext {
    private Request request;
    private Response response;
    private Map<String, String> routeParameters;
    private final Map<String, Collection<String>> queryParameters;
    private InputStream inputOverride;

    public HttpContext(Request request, Response response) {
        this(request, response, new HashMap<>());
    }

    public HttpContext(Request request, Response response, Map<String, String> routeParameters) {
        this.request = request;
        this.response = response;
        this.routeParameters = routeParameters;
        MultiMap<String> mm = new MultiMap<>();
        request.getUri().decodeQueryTo(mm);
        queryParameters = mm.keySet().stream()
                .collect(Collectors.toMap(k -> k, k -> (Collection) mm.getValues(k)));
    }


    public HttpContext withInputWrapper(InputStream inputStream) {
        HttpContext newCtx = new HttpContext(request, response, routeParameters);
        if(inputStream != null)
            newCtx.inputOverride = inputStream;
        return newCtx;
    }

    public HttpContext withRouteParameters(Map<String, String> routeParameters) {
        if(inputOverride == null)
            return new HttpContext(request, response, routeParameters);
        else
            return new HttpContext(request, response, routeParameters).withInputWrapper(inputOverride);
    }

    public Map<String, String> getRouteParameters() {
        return routeParameters;
    }

    public void setResponseContentType(String contentType) {
        response.setContentType(contentType);
    }

    public void setResponseStatus(int status) {
        response.setStatus(status);
    }

    public String getRequestHeader(String headerName) {
        return request.getHeader(headerName);
    }

    public String getRequestMethod() {
        return request.getMethod();
    }

    public String getRequestURI() {
        return request.getRequestURI();
    }

    public Collection<String> getRequestHeaderNames() {
        return Collections.list(request.getHeaderNames());
    }

    public Collection<String> getRequestHeaderValues(String header) {
        String value = request.getHeader(header);
        if (value == null) {
            return Lists.newArrayList();
        }
        return Lists.newArrayList(value);
    }

    public Map<String, Collection<String>> getQueryParameters() {
        return queryParameters;
    }

    public String getRequestContentType() {
        return request.getContentType();
    }

    public void addResponseHeader(String key, String value) {
        response.setHeader(key, value);
    }

    public String getRouteParameter(String routeParameter) {
        if (!routeParameters.containsKey(routeParameter)) {
            throw new IllegalArgumentException("Missing route parameter '" + routeParameter + "'");
        }

        return routeParameters.get(routeParameter);
    }

    public void addCookie(Cookie cookie) {
        response.addCookie(cookie);
    }

    public OutputStream getOutputStream() {
        try {
            return response.getOutputStream();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public InputStream getInputStream() {
        try {
            if(inputOverride == null)
                return request.getInputStream();
            else
                return inputOverride;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
