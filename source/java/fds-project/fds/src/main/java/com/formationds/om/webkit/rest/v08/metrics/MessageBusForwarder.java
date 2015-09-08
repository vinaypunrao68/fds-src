package com.formationds.om.webkit.rest.v08.metrics;

import java.io.InputStreamReader;
import java.io.StringWriter;
import java.net.URLDecoder;
import java.util.Map;

import org.apache.commons.httpclient.HttpClient;
import org.apache.commons.httpclient.HttpMethod;
import org.apache.commons.httpclient.methods.DeleteMethod;
import org.apache.commons.httpclient.methods.GetMethod;
import org.apache.commons.httpclient.methods.PostMethod;
import org.apache.commons.httpclient.methods.PutMethod;
import org.eclipse.jetty.server.Request;

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
		
		switch( request.getMethod() ){
			case "POST":
				PostMethod pMethod = new PostMethod( url );
				pMethod.setRequestBody( request.getInputStream() );
				method = pMethod;
				break;
			case "PUT":
				PutMethod putMethod = new PutMethod( url );
				putMethod.setRequestBody( request.getInputStream() );
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
		
		method.setRequestHeader( "Authorization", request.getHeader( "Authorization") );
		
		int statusCode = client.executeMethod( method );
		String body = new String( method.getResponseBody() );
		
		TextResource resource = new TextResource( statusCode, body );
		
		return resource;
	}

}
