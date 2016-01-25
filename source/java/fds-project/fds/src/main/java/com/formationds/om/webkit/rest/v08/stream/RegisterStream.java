/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.stream;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class RegisterStream implements RequestHandler {

    private ConfigurationApi configApi;

    public RegisterStream() {
    }

    @Override
    public Resource handle( Request request, Map<String, String> routeParameters )
            throws Exception {

        try ( InputStream is = request.getInputStream() ) {
            String source = IOUtils.toString( is );
            JSONObject o = new JSONObject( source );
            String url = o.getString( "url" );
            String httpMethod = o.getString( "method" );
            List<String> volumeNames = asList( o.getJSONArray( "volumes" ) );

            int sampleFreqSeconds = o.getInt( "frequency" );
            int durationSeconds = o.getInt( "duration" );

            int id = getConfigApi().registerStream( url, httpMethod, volumeNames, sampleFreqSeconds, durationSeconds );
            JSONObject result = new JSONObject().put( "id", id );
            return new JsonResource( result );
        }
    }

    private List<String> asList( JSONArray jsonArray ) {
        ArrayList<String> result = new ArrayList<>();
        for( int i = 0;
                i < jsonArray.length();
                i++ ) {
            result.add( jsonArray.getString( i ) );
        }
        return result;
    }

    private ConfigurationApi getConfigApi(){
        if ( configApi == null ){
            configApi = SingletonConfigAPI.instance().api();
        }

        return configApi;
    }
}
