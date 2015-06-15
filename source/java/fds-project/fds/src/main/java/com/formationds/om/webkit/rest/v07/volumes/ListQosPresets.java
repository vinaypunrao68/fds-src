package com.formationds.om.webkit.rest.v07.volumes;

import com.formationds.commons.model.QosPreset;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListQosPresets implements RequestHandler {

	/**
	 * This method is totally hard coded right now because we are not yet storing  presets
	 * or have them in an editable fashion.  However, by hard coding them here we can
	 * allow the UI and CLI to utilize them as if they were dynamic.
	 */
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
		
		List<QosPreset> qosPresets = new ArrayList<QosPreset>();
		
		QosPreset least = new QosPreset();
		least.setName( "Least Important" );
		least.setPriority( 10 );
		least.setLimit( 0L );
		least.setSla( 0L );
		least.setUuid( 1L );
		
		QosPreset standard = new QosPreset();
		standard.setName( "Standard" );
		standard.setPriority( 7 );
		standard.setLimit( 0L );
		standard.setSla( 0L );
		standard.setUuid( 2L );
		
		QosPreset most = new QosPreset();
		most.setName( "Most Important" );
		most.setPriority( 1 );
		most.setLimit( 0L );
		most.setSla( 0L );
		most.setUuid( 3L );
		
		qosPresets.add( least );
		qosPresets.add( standard );
		qosPresets.add( most );
		
		return new TextResource( ObjectModelHelper.toJSON( qosPresets ) );
	}
	
}
