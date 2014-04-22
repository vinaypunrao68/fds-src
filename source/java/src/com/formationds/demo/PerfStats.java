package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class PerfStats implements RequestHandler {
    public PerfStats(TransientState state) {
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        return new TextResource(canned) {
            @Override
            public String getContentType() {
                return "application/json";
            }
        };
    }

    private static String canned =
            "\n" +
                    "       [\n" +
                    "          {\n" +
                    "             operation: \"Read performance\",\n" +
                    "             unit: \"IOPs/second\"\n" +
                    "             values:\n" +
                    "             {\n" +
                    "                \"Volume1\": 43,\n" +
                    "                \"Volume2\": 5,\n" +
                    "                \"Volume3\": 12,\n" +
                    "             }\n" +
                    "          },\n" +
                    "          {\n" +
                    "             operation: \"Write performance\",\n" +
                    "             unit: \"IOPs/second\"\n" +
                    "             values:\n" +
                    "             {\n" +
                    "                \"Volume1\": 43,\n" +
                    "                \"Volume2\": 5,\n" +
                    "                \"Volume3\": 12,\n" +
                    "             }\n" +
                    "          }\n" +
                    "       ]";
}
