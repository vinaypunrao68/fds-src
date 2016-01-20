/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.events;

import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.RequestLog;
import com.formationds.web.toolkit.Resource;
import com.google.gson.reflect.TypeToken;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletRequest;

/**
 * Log an event in the OM event repository.
 * <p/>
 * Logging an event should be restricted to internal AM processes.  Modification of events from the UI or any other
 * client are not permitted.
 * TODO: how to enforce that restriction?
 */
public class IngestEvents implements RequestHandler {

    private static final Logger logger = LoggerFactory.getLogger(IngestEvents.class);
    private static final Type TYPE = new TypeToken<List<Event>>(){}.getType();

    /**
     */
    public IngestEvents() {
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        HttpServletRequest requestLoggingProxy = RequestLog.newRequestLogger( request );
        try ( final InputStreamReader reader = new InputStreamReader( requestLoggingProxy.getInputStream(), "UTF-8" ) ) {

            final List<Event> events = ObjectModelHelper.toObject(reader, TYPE);
            for( final Event event : events) {

                logger.trace("AM_EVENT: {}", event);

                // TODO replace with inject
                SingletonRepositoryManager.instance()
                                          .getEventRepository()
                                          .save( event );
            }
        }

        return new JsonResource( new JSONObject().put( "status", "OK" ) );
    }
}
