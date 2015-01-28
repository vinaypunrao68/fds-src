package com.formationds.spike.later;

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.route.LexicalTrie;
import com.formationds.web.toolkit.route.QueryResult;
import org.eclipse.jetty.server.Request;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Predicate;

public class HttpPath {
    private final List<Predicate<Request>> predicates;
    private final Map<String, String> routeParameters;

    public HttpPath() {
        predicates = new ArrayList<>();
        routeParameters = new HashMap<>();
    }

    public HttpPath(HttpMethod method, String routePattern) {
        this();
        withMethod(method);
        withPath(routePattern);
    }

    public HttpPath withMethod(HttpMethod method) {
        predicates.add(request -> request.getMethod().toLowerCase().equals(method.name().toLowerCase()));
        return this;
    }

    public HttpPath withPath(String pattern) {
        predicates.add(request -> {
            String path = request.getRequestURI()
                    .replaceAll("/$", "")
                    .replaceAll("^", "root:");

            LexicalTrie<Integer> trie = LexicalTrie.newTrie();
            trie.put(pattern.replaceAll("/$", "").replaceAll("^", "root:"), 0);
            QueryResult<Integer> result = trie.find(path);

            if (result.found()) {
                routeParameters.putAll(result.getMatches());
                return true;
            } else {
                return false;
            }
        });

        return this;
    }

    public HttpPath withUrlParam(String paramName) {
        predicates.add(request -> request.getQueryParameters().containsKey(paramName));
        return this;
    }

    public HttpPath withHeader(String headerName) {
        predicates.add(request -> request.getHeader(headerName) != null);
        return this;
    }


    public boolean matches(Request request) {
        if (predicates.size() == 0) {
            return false;
        }

        for (Predicate<Request> predicate : predicates) {
            if (!predicate.test(request)) {
                return false;
            }
        }

        return true;
    }

    public Map<String, String> getRouteParameters() {
        return routeParameters;
    }
}
