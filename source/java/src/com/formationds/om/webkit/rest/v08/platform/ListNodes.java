/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.client.v08.model.Node;
import com.formationds.commons.model.helper.ObjectModelHelper;
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
import java.util.stream.Collectors;

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
        
    	// TODO:  Fix this when there are more domains
    	List<com.formationds.protocol.FDSP_Node_Info_Type> list = getConfigApi().listLocalDomainServices("");
    	
    	logger.debug("Size of service list: {}", list.size());

    	Map<Long, List<FDSP_Node_Info_Type>> groupedServices = list.stream().collect( Collectors.groupingBy( FDSP_Node_Info_Type::getNode_uuid) );
    	
    	List<Node> nodes = new ArrayList<>();
    	
    	groupedServices.keySet().stream().forEach( nodeId -> {
    		
    		try {
				Node node = PlatformModelConverter.convertToExternalNode( groupedServices.get( nodeId ) );
				nodes.add( node );
				
			} catch (Exception e) {
				logger.warn( "Could not create a node object for uuid: " + nodeId, e );
			}
    	});
    	
    	return nodes;
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }


}
