/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest.events;

import com.formationds.commons.model.Events;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.helper.QueryHelper;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.gson.reflect.TypeToken;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.util.Map;

/**
 * @author dsetzke based on QueryMetrics by ptinius
 */
public class QueryEvents implements RequestHandler {

    private static final Type TYPE = new TypeToken<QueryCriteria>() { }.getType();

    public QueryEvents() {
        super();
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {

        try (final Reader reader = new InputStreamReader(request.getInputStream(), "UTF-8")) {
            final QueryCriteria eventQuery = ObjectModelHelper.toObject(reader, TYPE);
            final Events events = new QueryHelper().executeEventQuery(eventQuery);

            return new JsonResource(new JSONObject(events));
        }
    }
}
