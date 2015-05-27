package com.formationds.om.webkit.rest.v08.volumes;

import java.util.Map;

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;

public class GetVolume  implements RequestHandler {
	
    private static final Logger LOG = Logger.getLogger(ListVolumes.class);

   
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
       
		return new TextResource("ok");
	}
}
