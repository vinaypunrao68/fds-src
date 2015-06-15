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
    private DemoState state;

    public PerfStats(DemoState state) {
        this.state = state;
    }

/*
       Returns a jason frame like so:

       [
          {
             operation: "Read performance",
             unit: "IOPs/second"
             values:
             {
                "Volume1": 43,
                "Volume2": 5,
                "Volume3": 12,
             }
          },
          {
             operation: "Write performance",
             unit: "IOPs/second"
             values:
             {
                "Volume1": 43,
                "Volume2": 5,
                "Volume3": 12,
             }
          }
       ]
*/

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        BucketStats readBucketStats = state.readCounts();
        BucketStats writeBucketStats = state.writeCounts();


        JSONArray array = new JSONArray()
                .put(makeSummary(readBucketStats, "Read performance", "Images read"))
                .put(makeSummary(writeBucketStats, "Write performance", "Images written"));

        return new JsonResource(array);
//        return new TextResource(canned) {
//            @Override
//            public String getContentType() {
//                return "application/json";
//            }
//        };
    }

    private JSONObject makeSummary(BucketStats bucketStats, String legend, String unitName) {
        return new JSONObject()
                .put("operation", legend)
                .put("unit", unitName)
                .put("values", asJson(bucketStats));
    }

    private JSONObject asJson(BucketStats bucketStats) {
        JSONObject o = new JSONObject();
        for (int i = 0; i < Main.VOLUMES.length; i++) {
            String volume = Main.VOLUMES[i];
            o.put(volume, bucketStats.get(volume).n());
        }
        return o;
    }

    private static String canned =
           "[\n" +
                   "    {\n" +
                   "        \"unit\": \"Images read\",\n" +
                   "        \"values\": {\n" +
                   "            \"fdspanda\": 25,\n" +
                   "            \"fdsfrog\": 30\n" +
                   "        },\n" +
                   "        \"operation\": \"Read performance\"\n" +
                   "    },\n" +
                   "    {\n" +
                   "        \"unit\": \"Images written\",\n" +
                   "        \"values\": {\n" +
                   "            \"fdspanda\": 25,\n" +
                   "            \"fdsfrog\": 30\n" +
                   "        },\n" +
                   "        \"operation\": \"Write performance\"\n" +
                   "    }\n" +
                   "]\n";
}
