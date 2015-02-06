package com.formationds.spike.later;

import com.formationds.spike.later.pathtemplate.PathTemplate;
import com.formationds.spike.later.pathtemplate.RouteSignature;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.route.LexicalTrie;
import com.formationds.web.toolkit.route.QueryResult;
import io.undertow.server.HttpServerExchange;

import java.util.*;
import java.util.function.Predicate;

public class HttpPath {
    private HttpMethod method;
    private PathTemplate pathTemplate;
    private final List<Predicate<HttpServerExchange>> predicates;

    public HttpPath() {
        predicates = new ArrayList<>();
    }

    public HttpPath(HttpMethod method, String routePattern) {
        this();
        withMethod(method);
        withPath(routePattern);
    }

    public HttpPath withMethod(HttpMethod method) {
        this.method = method;
        return this;
    }

    public HttpPath withPath(String pattern) {
        pathTemplate = PathTemplate.parseSimple(pattern);
        return this;
    }

    public HttpPath withUrlParam(String paramName) {
        predicates.add(request -> request.getQueryParameters().containsKey(paramName));
        return this;
    }

    public HttpPath withHeader(String headerName) {
        predicates.add(request -> request.getRequestHeaders().getFirst(headerName) != null);
        return this;
    }


    public MatchResult matches(HttpServerExchange exchange) {
        if (predicates.size() == 0 && method == null && pathTemplate == null) {
            return failedMatch;
        }

        if(method != null && !exchange.getRequestMethod().toString().equalsIgnoreCase(method.toString()))
            return failedMatch;

        Map<String, String> pathParams = null;
        if(pathTemplate != null) {
            //String path = exchange.getRequestPath().replaceAll("/$", "");
            PathTemplate.TemplateMatch match = pathTemplate.match(exchange.getRequestPath());
            if(!match.isMatch())
                return failedMatch;
            pathParams = match.getParameters();
        } else {
            pathParams = Collections.emptyMap();
        }

        for (Predicate<HttpServerExchange> predicate : predicates) {
            if (!predicate.test(exchange)) {
                return failedMatch;
            }
        }

        return new MatchResult(true, pathParams);
    }

    public Optional<RouteSignature> routeSignature() {
        if(pathTemplate != null)
            return Optional.of(new RouteSignature(method, pathTemplate.getPathTemplateElements().size()));
        return Optional.empty();
    }

    private static final MatchResult failedMatch = new MatchResult(false, null);
    public static class MatchResult {
        private boolean isMatch;
        private Map<String, String> pathParameters;

        private MatchResult(boolean isMatch, Map<String, String> pathParameters) {
            this.isMatch = isMatch;
            this.pathParameters = pathParameters;
        }

        public boolean isMatch() {
            return isMatch;
        }

        public Map<String, String> getPathParameters() {
            return pathParameters;
        }
    }
}
