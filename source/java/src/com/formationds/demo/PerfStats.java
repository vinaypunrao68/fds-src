package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Map;

public class PerfStats implements RequestHandler {
    private TransientState state;

    public PerfStats(TransientState state) {
        this.state = state;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        Counts readCounts = state.consumeReadCounts();
        Counts writeCounts = state.consumeWriteCounts();

        JSONArray array = new JSONArray()
                .put(makeSummary(readCounts, "Read performance", "Reads/second"))
                .put(makeSummary(writeCounts, "Write performance", "Writes/second"));

        return new JsonResource(array);
    }

    private JSONObject makeSummary(Counts counts, String legend, String unitName) {
        return new JSONObject()
                .put("operation", legend)
                .put("unit", unitName)
                .put("values", asJson(counts));
    }

    private JSONObject asJson(Counts counts) {
        JSONObject o = new JSONObject();
        counts.keys().forEach(k -> {
            o.put(k, counts.get(k));
        });
        return o;
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
