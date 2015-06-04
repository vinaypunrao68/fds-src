package com.formationds.om.webkit.rest.v08.volumes;

import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class CloneVolumeFromTime implements RequestHandler{

	private static final String VOLUME_ARG = "volume_id";
	private static final String TIME_ARG = "time_in_seconds";
	
	private ConfigurationApi configApi;
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		long volumeId = requiredLong( routeParameters, VOLUME_ARG );
		long timeInSeconds = requiredLong( routeParameters, TIME_ARG );
		
		return null;
	}
	
	public ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}

}
