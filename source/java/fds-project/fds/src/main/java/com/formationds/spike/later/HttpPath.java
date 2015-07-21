package com.formationds.spike.later;

import com.formationds.spike.later.pathtemplate.PathTemplate;
import com.formationds.spike.later.pathtemplate.RouteSignature;
import com.formationds.web.toolkit.HttpMethod;

import java.util.*;
import java.util.function.Predicate;

public class HttpPath {
    private HttpMethod method;
    private PathTemplate pathTemplate;
    private List<Predicate<HttpContext>> predicates;

    public HttpPath() {
        predicates = new ArrayList<>();
    }

    public HttpPath(HttpMethod method, String routePattern) {
        this();
        withMethod(method);
        withPath(routePattern);
    }

    public HttpPath clone() {
        HttpPath path = new HttpPath();
        path.method = method;
        path.pathTemplate = pathTemplate;
        path.predicates = new ArrayList<>(predicates);
        return path;
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
        predicates.add(request -> request.getRequestHeader(headerName) != null);
        return this;
    }

    public HttpPath withPredicate(Predicate<HttpContext> predicate) {
        predicates.add(predicate);
        return this;
    }


    public MatchResult matches(HttpContext context) {
        if (predicates.size() == 0 && method == null && pathTemplate == null) {
            return failedMatch;
        }

        if (method != null && !context.getRequestMethod().toString().equalsIgnoreCase(method.toString()))
            return failedMatch;

        Map<String, String> pathParams = null;
        if(pathTemplate != null) {
            //String path = exchange.getRequestPath().replaceAll("/$", "");
            PathTemplate.TemplateMatch match = pathTemplate.match(context.getRequestURI());
            if(!match.isMatch())
                return failedMatch;
            pathParams = match.getParameters();
        } else {
            pathParams = Collections.emptyMap();
        }

        for (Predicate<HttpContext> predicate : predicates) {
            if (!predicate.test(context)) {
                return failedMatch;
            }
        }

        return new MatchResult(true, pathParams);
    }

    public Optional<RouteSignature> routeSignature() {
        if(pathTemplate != null && method != null)
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
