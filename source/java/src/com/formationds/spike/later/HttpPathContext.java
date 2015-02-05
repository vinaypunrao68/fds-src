package com.formationds.spike.later;

import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;

import java.util.Map;

public class HttpPathContext {
    private Request request;
    private Response response;
    private Map<String, String> routeParameters;

    public HttpPathContext(Request request, Response response, Map<String, String> routeParameters) {
        this.request = request;
        this.response = response;
        this.routeParameters = routeParameters;
    }

    public Request getRequest() {
        return request;
    }

    public Response getResponse() {
        return response;
    }

    public Map<String, String> getRouteParameters() {
        return routeParameters;
    }
}
