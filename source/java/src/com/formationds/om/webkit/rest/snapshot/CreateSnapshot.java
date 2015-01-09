/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.snapshot;

import com.formationds.commons.model.Snapshot;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

public class CreateSnapshot
  implements RequestHandler {
  private com.formationds.util.thrift.ConfigurationApi config;

    public CreateSnapshot(com.formationds.util.thrift.ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
        throws Exception {

        try (final Reader reader =
                 new InputStreamReader(request.getInputStream(), "UTF-8")) {
            Gson gson = new GsonBuilder().create();
            final Snapshot snapshot = gson.fromJson(reader,
                                                    Snapshot.class);

            config.createSnapshot(Long.valueOf(snapshot.getVolumeId()),
                                  snapshot.getName(),
                                  snapshot.getRetention(),
                                  snapshot.getTimelineTime() );
    }

    return new JsonResource( new JSONObject().put( "status", "OK" ) );
  }
}
