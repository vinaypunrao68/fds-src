/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.platform;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_RemoveServicesType;
import FDS_ProtocolInterface.FDSP_Uuid;
import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventDescriptor;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.commons.model.Node;
import com.formationds.om.events.EventManager;
import com.formationds.util.thrift.pm.PMServiceClient;
import com.formationds.util.thrift.pm.PMServiceException;
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
import java.util.Optional;

public class DeactivateNode
    implements RequestHandler {

    private static final Logger logger =
        LoggerFactory.getLogger( DeactivateNode.class );

    private FDSP_ConfigPathReq.Iface client;

    public DeactivateNode( FDSP_ConfigPathReq.Iface client ) {

        this.client = client;

    }

    @Override
    public Resource handle( Request request, Map<String, String> routeParameters )
        throws Exception {

        final long nodeUuid = requiredLong( routeParameters, "node_uuid" );
        final Optional<String> nodeName =
            nodeName( String.valueOf( nodeUuid ) );

        if( !nodeName.isPresent() ) {

            throw new Exception( "The specified node uuid " + nodeUuid +
                                 " has no matching node name." );

        }

        String source = IOUtils.toString( request.getInputStream() );

        JSONObject o = new JSONObject( source );

        boolean activateSm = !o.isNull( "sm" ) && o.getBoolean( "sm" );
        boolean activateAm = !o.isNull( "am" ) && o.getBoolean( "am" );
        boolean activateDm = !o.isNull( "dm" ) && o.getBoolean( "dm" );

        logger.debug( "Deactivating {}:{} AM: {} DM: {} SM: {}",
                      nodeName.get(),
                      nodeUuid,
                      activateAm,
                      activateDm,
                      activateSm );

        int status =
            client.RemoveServices( new FDSP_MsgHdrType(),
                                   new FDSP_RemoveServicesType(
                                     nodeName.get(),
                                     new FDSP_Uuid( nodeUuid ),
                                     activateSm,
                                     activateDm,
                                     activateAm ) );

        int httpCode = HttpServletResponse.SC_OK;
        if( status != 0 ) {

            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( DeactivateNodeEvent.DEACTIVATE_NODE_ERROR,
                                      nodeName,
                                      nodeUuid );

        } else {

            EventManager.notifyEvent( DeactivateNodeEvent.DEACTIVATE_NODE,
                                      nodeName,
                                      nodeUuid );

        }

        return new JsonResource( new JSONObject().put( "status", status ),
                                 httpCode );
    }

    protected Optional<String> nodeName( final String nodeUuid ) {

        // refresh
        // connect to local PM to get all node uuid to node name
        final PMServiceClient client = new PMServiceClient( );
        try {

            for( final List<Node> nodes : client.getDomainNodes().values() ) {

                for( final Node node : nodes ) {

                    if( node.getUuid().equalsIgnoreCase( nodeUuid ) ) {
                        return Optional.of( node.getName() );
                    }
                }

            }

        } catch( PMServiceException e ) {

            logger.error( e.getMessage(), e );
        }

        return Optional.empty();
    }

    // TODO Move event packages
    enum DeactivateNodeEvent
        implements EventDescriptor {

        DEACTIVATE_NODE( EventCategory.SYSTEM,
                         "Deactivating node {0}:{1}",
                         "nodeName",
                         "nodeUuid" ),
        DEACTIVATE_NODE_ERROR( EventType.SYSTEM_EVENT,
                               EventCategory.SYSTEM,
                               EventSeverity.ERROR,
                               "Deactivating node {0}:{1} failed.",
                               "nodeName",
                               "nodeUuid" );

        private final EventType type;
        private final EventCategory category;
        private final EventSeverity severity;
        private final String        defaultMessage;
        private final List<String> argNames;

        private DeactivateNodeEvent( final EventCategory category,
                                     final String defaultMessage,
                                     final String... argNames) {

            this( EventType.SYSTEM_EVENT,
                  category,
                  EventSeverity.CONFIG,
                  defaultMessage,
                  argNames );
        }

        private DeactivateNodeEvent( final EventType type,
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
