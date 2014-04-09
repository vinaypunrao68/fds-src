package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class ShowVolumeDefaults implements RequestHandler {
    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String retValue = "\n" +
                "[  \"volume\": {\n" +
                "    \"data_connectors\": [\n" +
                "      {\n" +
                "        \"type\": \"block\",\n" +
                "        \"attributes\": {\n" +
                "        \"unit\": \"TB\",\n" +
                "        \"size\": \"3\"\n" +
                "      },\n" +
                "      {\"type\": \"S3\"},\n" +
                "      {\"type\": \"Native\"}\n" +
                "]\n";

        return new TextResource(retValue) {
            @Override
            public String getContentType() {
                return "application/json";
            }
        };
    }
}
