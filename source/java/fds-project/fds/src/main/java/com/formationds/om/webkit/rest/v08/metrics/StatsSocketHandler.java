package com.formationds.om.webkit.rest.v08.metrics;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import com.formationds.stats_client.model.StatDataPoint;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class StatsSocketHandler implements RequestHandler{

	private static final String LAST_TIME = "lastTime";
	
	public StatsSocketHandler() {
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		String lastTimeStr = requiredString( request, LAST_TIME );
		
		final Long lastTime = Long.parseLong( lastTimeStr );
		
		List<StatDataPoint> points = StatsStream.getInstance().getStatCache().asMap().values().stream().filter( ( datapoint ) -> {
			
			if ( datapoint.getReportTime() > lastTime ){
				return true;
			}
			
			return false;
		}).collect( Collectors.toList() );
		
		JSONArray jArr = new JSONArray( points );
		JSONObject rtn = new JSONObject();
		rtn.put( "newPoints",  jArr );
		
		return new TextResource( rtn.toString() );
	}

}
