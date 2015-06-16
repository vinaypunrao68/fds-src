package com.formationds.om.webkit.rest.v08.presets;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.client.v08.model.QosPolicyPreset;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class GetQosPolicyPresets implements RequestHandler{

	private static final Logger logger = LoggerFactory.getLogger( GetQosPolicyPresets.class );
	private List<QosPolicyPreset> policyPresets;
	private QosPolicyPreset lowPriorityPreset;
	private QosPolicyPreset normalPriorityPreset;
	private QosPolicyPreset highPriorityPreset;
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		logger.debug( "Requesting all the QOS preset policies." );
		
		List<QosPolicyPreset> presets = getQosPolicyPresets();
		
		logger.debug( "Found {} policies.", presets.size() );
		
		String jsonString = ObjectModelHelper.toJSON( presets );
		
		return new TextResource( jsonString );
	}
	
	public List<QosPolicyPreset> getQosPolicyPresets(){
		
		if ( policyPresets == null ){
			policyPresets = new ArrayList<>();
			
			policyPresets.add( getLowPriorityPreset() );
			policyPresets.add( getNormalPriorityPreset() );
			policyPresets.add( getHighPriorityPreset() );
		}
		
		return policyPresets;
	}
	
	private QosPolicyPreset getLowPriorityPreset(){
		
		if ( lowPriorityPreset == null ){
			lowPriorityPreset = new QosPolicyPreset( 0L, "Least Important", 10, 0, 0 );
		}
		
		return lowPriorityPreset;
	}
	
	private QosPolicyPreset getNormalPriorityPreset(){
		
		if ( normalPriorityPreset == null ){
			normalPriorityPreset = new QosPolicyPreset( 1L, "Standard", 7, 0, 0 );
		}
		
		return normalPriorityPreset;
	}
	
	private QosPolicyPreset getHighPriorityPreset(){
		
		if ( highPriorityPreset == null ){
			highPriorityPreset = new QosPolicyPreset( 2L, "Most Important", 1, 0, 0 );
		}
		
		return highPriorityPreset;
	}
}
