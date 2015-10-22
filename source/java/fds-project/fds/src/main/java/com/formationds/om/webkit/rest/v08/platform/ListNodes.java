/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.client.v08.model.Node;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.util.NodeUtils;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import com.formationds.util.Configuration;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListNodes
    implements RequestHandler {

    private static final Logger logger =
        LoggerFactory.getLogger( ListNodes.class );

    private ConfigurationApi configApi;

    public ListNodes(){}

    public Resource handle( final Request request,
                            final Map<String, String> routeParameters )
        throws Exception {

    	List<Node> nodes = getNodes();
    	
    	String jsonString = ObjectModelHelper.toJSON( nodes );

        return new TextResource( jsonString );
    }
    
    public List<Node> getNodes() throws TException{
        
    	logger.debug( "Getting a list of nodes." );
    	
    	// FIXME: Fix this when there are more domains
    	List<FDSP_Node_Info_Type> list = getConfigApi().ListServices(0);

    	logger.debug( "Size of service list: {}", list.size( ) );
    	for ( FDSP_Node_Info_Type aNode : list )
    	{
    		logger.debug( "Node name: {} uuid: {} ", aNode.getNode_name(), aNode.getNode_uuid() );
    	}

        Map<Long, List<FDSP_Node_Info_Type>> groupedServices =
            NodeUtils.groupNodes( list, getConfiguration().getOMNodeUuid() );

    	final List<Node> nodes = new ArrayList<>();
        for( final Long nodeIp : groupedServices.keySet( ) )
        {
            List<FDSP_Node_Info_Type> services = groupedServices.get( nodeIp );

            try
            {
                Node node =
                    PlatformModelConverter.convertToExternalNode( services );
                nodes.add( node );

            }
            catch( Exception e )
            {
                logger.warn( "Could not create a node object for IP: " +
                             nodeIp, e );
            }
        }

    	return nodes;
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }

    private Configuration getConfiguration() {
        return SingletonConfiguration.instance( )
                                     .getConfig();
    }
}
