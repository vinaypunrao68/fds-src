/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

import com.formationds.client.v08.model.Node;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class GetNode implements RequestHandler{

	private static final String NODE_ARG = "node_id";
	
    private static final Logger logger =
            LogManager.getLogger( AddService.class );

	
	public GetNode(){}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		long nodeId = requiredLong( routeParameters, NODE_ARG );
		
		logger.debug( "Trying to retrieve node: " + nodeId );
		
		Node node = getNode( nodeId );
		
		String jsonString = ObjectModelHelper.toJSON( node );
		
        return new TextResource( jsonString );
	}
	
	public Node getNode( long nodeId ) throws ApiException, TException {
		
		List<Node> nodes = (new ListNodes()).getNodes();
		
		Optional<Node> foundNode = nodes.stream().filter( (node) -> {
			
			if ( node.getId().equals( nodeId ) ){
				return true;
			}
			
			return false;
			
		}).findFirst();
		
		if ( foundNode.isPresent() ){
			return foundNode.get();
		}
		
		throw new ApiException( "Could not locate the node with ID: " + nodeId, ErrorCode.MISSING_RESOURCE );
	}
	
}
