package com.formationds.om.webkit.rest.v08.metrics;

import java.net.URLDecoder;
import java.util.Base64;
import java.util.Map;

import org.apache.commons.httpclient.HttpClient;
import org.apache.commons.httpclient.HttpMethod;
import org.apache.commons.httpclient.methods.DeleteMethod;
import org.apache.commons.httpclient.methods.GetMethod;
import org.apache.commons.httpclient.methods.PostMethod;
import org.apache.commons.httpclient.methods.PutMethod;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;

import com.formationds.om.webkit.rest.v08.users.GetUser;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class MessageBusForwarder implements RequestHandler {

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		HttpClient client = new HttpClient();
		String mbRoute = URLDecoder.decode( requiredString( routeParameters, "route" ), "UTF-8" );
		
		HttpMethod method;
		String url = "http://localhost:15672/" + mbRoute;
		String data = IOUtils.toString( request.getInputStream() );
		
		switch( request.getMethod() ){
			case "POST":
				PostMethod pMethod = new PostMethod( url );
				pMethod.setRequestBody( data );
				method = pMethod;
				break;
			case "PUT":
				PutMethod putMethod = new PutMethod( url );
				putMethod.setRequestBody( data );
				method = putMethod;
				break;
			case "DELETE":
				method = new DeleteMethod( url );
				break;
			case "GET":
			default:
				method = new GetMethod( url );
				break;
		}
		
		method.setRequestHeader( "Authorization", "Basic " + new String( Base64.getEncoder().encode( (GetUser.STATS_USERNAME + ":$t@t$").getBytes() ) ) );
		method.setRequestHeader( "Content-Type", "application/json" );
		
		int statusCode = client.executeMethod( method );
		String body = "";
				
		// our code doesn't see a 204 as a success ... but for the message bus it is.
		if ( statusCode == 204 ){
			statusCode = 200;
		}
		else {
			body = method.getResponseBodyAsString();
		}
		
		TextResource resource = new TextResource( statusCode, body );
		
		return resource;
	}

}
