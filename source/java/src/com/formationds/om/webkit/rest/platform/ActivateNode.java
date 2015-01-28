package com.formationds.om.webkit.rest.platform;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ActivateOneNodeType;
import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Uuid;
import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventDescriptor;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.om.events.EventManager;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

public class ActivateNode
    implements RequestHandler {

    private static final Logger logger =
        LoggerFactory.getLogger( ActivateNode.class );

    private FDSP_ConfigPathReq.Iface client;

    public ActivateNode( final FDSP_ConfigPathReq.Iface client ) {

        this.client = client;

    }

    @Override
    public Resource handle( final Request request,
                            final Map<String, String> routeParameters)
        throws Exception {

        long nodeUuid = requiredLong(routeParameters, "node_uuid");

        String source = IOUtils.toString(request.getInputStream());

        JSONObject o = new JSONObject(source);

        boolean activateSm = !o.isNull( "sm" ) && o.getBoolean( "sm" );
        boolean activateAm = !o.isNull( "am" ) && o.getBoolean( "am" );
        boolean activateDm = !o.isNull( "dm" ) && o.getBoolean( "dm" );

        logger.debug( "Activating {} AM: {} DM: {} SM: {}",
                      nodeUuid, activateAm, activateDm, activateSm );

        int status =
            client.ActivateNode( new FDSP_MsgHdrType(),
                                 new FDSP_ActivateOneNodeType(
                                     1,
                                     new FDSP_Uuid( nodeUuid ),
                                     activateSm,
                                     activateDm,
                                     activateAm ) );

        int httpCode = HttpServletResponse.SC_OK;
        if( status != 0 ) {

            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( ActivateNodeEvent.ACTIVATE_NODE_ERROR,
                                      nodeUuid );

        } else {

            EventManager.notifyEvent( ActivateNodeEvent.ACTIVATE_NODE,
                                      nodeUuid );

        }

        return new JsonResource( new JSONObject().put( "status", status ),
                                 httpCode);
    }

    enum ActivateNodeEvent
        implements EventDescriptor {

        ACTIVATE_NODE( EventCategory.SYSTEM,
                       "Activating node {0}",
                       "nodeUuid" ),
        ACTIVATE_NODE_ERROR( EventType.SYSTEM_EVENT,
                             EventCategory.SYSTEM,
                             EventSeverity.ERROR,
                             "Activating node uuid {0} failed.",
                             "nodeUuid" );

        private final EventType type;
        private final EventCategory category;
        private final EventSeverity severity;
        private final String        defaultMessage;
        private final List<String> argNames;

        private ActivateNodeEvent( final EventCategory category,
                                   final String defaultMessage,
                                   final String... argNames) {

            this( EventType.SYSTEM_EVENT,
                  category,
                  EventSeverity.CONFIG,
                  defaultMessage,
                  argNames );
        }

        private ActivateNodeEvent( final EventType type,
                                   final EventCategory category,
                                   final EventSeverity severity,
                                   final String defaultMessage,
                                   final String... argNames) {

            this.type = type;
            this.category = category;
            this.severity = severity;
            this.defaultMessage = defaultMessage;
            this.argNames = (argNames != null ? Arrays.asList(
                argNames ) : new ArrayList<>(0));
        }

        /**
         * @return the event type
         */
        @Override
        public EventType type( ) {
            return type;
        }

        /**
         * @return the event category
         */
        @Override
        public EventCategory category( ) {
            return category;
        }

        /**
         * @return the event severity
         */
        @Override
        public EventSeverity severity( ) {
            return severity;
        }

        /**
         * @return the default message
         */
        @Override
        public String defaultMessage( ) {
            return defaultMessage;
        }

        /**
         * @return the message argument names
         */
        @Override
        public List<String> argNames( ) {
            return argNames;
        }

        /**
         * @return the message key
         */
        @Override
        public String key( ) {
            return name();
        }
    }
}
