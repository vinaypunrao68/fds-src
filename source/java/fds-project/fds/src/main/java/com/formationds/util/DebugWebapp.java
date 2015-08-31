package com.formationds.util;

import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;
import com.formationds.web.toolkit.*;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.XdiConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;
import java.util.Map;

public class DebugWebapp {
    public void start(int serverPort, AsyncAm asyncAm, XdiConfigurationApi configurationApi) {
        WebApp webApp = new WebApp(".");
        webApp.route(HttpMethod.GET, "/:volume", () -> new ListVolume(asyncAm));
        webApp.start(new HttpConfiguration(serverPort));
    }

    class ListVolume implements RequestHandler {
        private AsyncAm asyncAm;

        public ListVolume(AsyncAm asyncAm) {
            this.asyncAm = asyncAm;
        }

        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            String volumeName = routeParameters.get("volume");
            List<BlobDescriptor> bds = asyncAm.volumeContents("",
                                                              volumeName,
                                                              100,
                                                              0,
                                                              "",
                                                              PatternSemantics.PCRE,
                                                              "",
                                                              BlobListOrder.UNSPECIFIED,
                                                              false).get();
            JSONArray array = new JSONArray();
            for (BlobDescriptor bd : bds) {
                JSONObject o = new JSONObject();
                o.put("name", bd.getName());
                o.put("size", bd.getByteCount());
                JSONObject metadata = new JSONObject();
                bd.getMetadata().keySet().forEach(k -> metadata.put(k, bd.getMetadata().get(k)));
                o.put("metadata", metadata);
                array.put(o);
            }
            return new JsonResource(array);
        }
    }
}
