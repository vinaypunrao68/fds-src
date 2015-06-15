package com.formationds.om.webkit.rest.v08.volumes;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeSettingsBlock;
import com.formationds.client.v08.model.VolumeSettingsObject;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class GetVolumeTypes implements RequestHandler{

	private static final Logger logger = LoggerFactory.getLogger( GetVolumeTypes.class );
	private List<VolumeSettings> volumeTypes;
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		logger.debug( "Getting supported volume types." );
		
		List<VolumeSettings> types = getVolumeTypes();
		
		logger.debug( "Found {} types.", types.size() );
		
		String jsonString = ObjectModelHelper.toJSON( types );
		
		return new TextResource( jsonString );
	}
	
	public List<VolumeSettings> getVolumeTypes(){
		
		if ( volumeTypes == null ){
			
			volumeTypes = new ArrayList<>();
			
			VolumeSettings blockType = new VolumeSettingsBlock(new Size( BigDecimal.valueOf( 10 ), SizeUnit.GB ));
			
			VolumeSettings objectType = new VolumeSettingsObject();
			
			volumeTypes.add( objectType );
			volumeTypes.add( blockType );
		}
		
		return volumeTypes;
	}

}
