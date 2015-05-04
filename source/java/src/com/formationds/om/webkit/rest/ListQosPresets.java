package com.formationds.om.webkit.rest;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.commons.model.QosPreset;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

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
		
		QosPreset standard = new QosPreset();
		least.setName( "Standard" );
		least.setPriority( 7 );
		least.setLimit( 0L );
		least.setSla( 0L );
		
		QosPreset most = new QosPreset();
		most.setName( "Most Important" );
		least.setPriority( 1 );
		least.setLimit( 0L );
		least.setSla( 0L );
		
		qosPresets.add( least );
		qosPresets.add( standard );
		qosPresets.add( most );
		
		return new TextResource( ObjectModelHelper.toJSON( qosPresets ) );
	}
	
}
