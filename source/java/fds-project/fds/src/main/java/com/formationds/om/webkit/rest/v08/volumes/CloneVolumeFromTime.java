package com.formationds.om.webkit.rest.v08.volumes;

import java.io.InputStreamReader;
import java.util.Map;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class CloneVolumeFromTime implements RequestHandler{

    private static final String VOLUME_ARG = "volume_id";
    private static final String TIME_ARG = "time_in_seconds";
    private static final Logger logger = LoggerFactory.getLogger( CloneVolumeFromTime.class );

    private ConfigurationApi configApi;
    private Authorizer authorizer;
    private AuthenticationToken token;

    public CloneVolumeFromTime( Authorizer authorizer, AuthenticationToken token ){
        this.authorizer = authorizer;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters)
            throws Exception {

        long volumeId = requiredLong( routeParameters, VOLUME_ARG );
        long timeInSeconds = requiredLong( routeParameters, TIME_ARG );

        logger.debug( "Cloning volume: {} from time: {}(sec).", volumeId, timeInSeconds );

        Volume newVolume = null;
        try (final InputStreamReader reader = new InputStreamReader( request.getInputStream() )) {
            newVolume = ObjectModelHelper.toObject( reader , Volume.class );
        }

        Volume clonedVolume = cloneFromTime( volumeId, newVolume, timeInSeconds );

        String jsonString = ObjectModelHelper.toJSON( clonedVolume );

        return new TextResource( jsonString );
    }

    public Volume cloneFromTime( long volumeId, Volume volume, long timeInSeconds ) throws Exception {

        long newId = getConfigApi().cloneVolume( volumeId, 0L, volume.getName(), timeInSeconds );
        volume.setId( newId );

        CreateVolume volumeEndpoint = new CreateVolume( getAuthorizer(), getToken() );

        volumeEndpoint.setQosForVolume( volume );

        volumeEndpoint.createSnapshotPolicies( volume );

        Volume newVolume = (new GetVolume( getAuthorizer(), getToken() ) ).getVolume( newId );

        return newVolume;
    }

    private Authorizer getAuthorizer(){
        return this.authorizer;
    }

    private AuthenticationToken getToken(){
        return this.token;
    }

    public ConfigurationApi getConfigApi(){

        if ( configApi == null ){
            configApi = SingletonConfigAPI.instance().api();
        }

        return configApi;
    }

}
