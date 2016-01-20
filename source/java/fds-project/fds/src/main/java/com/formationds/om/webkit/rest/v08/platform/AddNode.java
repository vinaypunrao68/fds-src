/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.pm.NotifyAddServiceMsg;
import com.formationds.protocol.pm.NotifyStartServiceMsg;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.RequestLog;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;


public class AddNode
    implements RequestHandler {

	private static final String NODE_ARG = "node_id";

    private static final Logger logger =
        LoggerFactory.getLogger( AddNode.class );

    private ConfigurationApi configApi;

    public AddNode() {}

    @Override
    public Resource handle( final Request request,
                            final Map<String, String> routeParameters)
        throws Exception {

        long nodeUuid = requiredLong(routeParameters, NODE_ARG );

        logger.debug( "Trying to add node: " + nodeUuid );

        Node node = null;
        try ( final InputStreamReader reader = new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {
            node = ObjectModelHelper.toObject( reader, Node.class );
        }

        if( node == null ) {
	  		throw new ApiException( "The specified node uuid " + nodeUuid +
	  				                " cannot be found", ErrorCode.MISSING_RESOURCE );
  		}

        List<SvcInfo> svcInfList = new ArrayList<SvcInfo>();
        boolean pmPresent = false;

        logger.trace( "NODE::" + node.toString() );
        for(List<Service> svcList : node.getServices().values())
        {
        	for(Service svc : svcList)
        	{
        		SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        				                                 (node.getAddress().getHostAddress(),
                                                          svc);
        		svcInfList.add(svcInfo);

        		if (svc.getType() == ServiceType.PM) {
        			pmPresent = true;
        		}

        	}
        }

        if (!pmPresent)
        {
        	Service pmSvc = (new GetService()).getService(nodeUuid, nodeUuid);
        	SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        			                                 (node.getAddress().getHostAddress(),
                                                      pmSvc);
        	svcInfList.add(svcInfo);
        }

        logger.debug("Adding and starting services on node");
        int status =
        		getConfigApi().AddService(new NotifyAddServiceMsg(svcInfList));

        if( status != 0 )
        {
            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.ADD_NODE_ERROR,
                                      nodeUuid );
        }
        else
        {
            // Now that we have added the services, go start them
            status = getConfigApi().StartService(new NotifyStartServiceMsg(svcInfList, true));

            if( status != 0 )
            {
                status= HttpServletResponse.SC_BAD_REQUEST;
                EventManager.notifyEvent( OmEvents.ADD_NODE_ERROR,
                                          nodeUuid );
            }
            else
            {
                EventManager.notifyEvent( OmEvents.ADD_NODE,
                                          nodeUuid );
            }
        }

        Node newNode = (new GetNode()).getNode( nodeUuid );

        String jsonString = ObjectModelHelper.toJSON( newNode );

        return new TextResource( jsonString );
    }

    private Boolean activateService( ServiceType type, Node node ){

    	List<Service> services = node.getServices().get( type );

    	if ( services.size() == 0 ){
    		return false;
    	}

    	return true;
    }

    private ConfigurationApi getConfigApi(){

    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}

    	return configApi;
    }
}
