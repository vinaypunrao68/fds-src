/*
 * Copyright (c) 2016 Formation Data Systems. All rights Reserved.
 */
package com.formationds.util.thrift;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.AuthenticatedUser;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.protocol.ErrorCode;
import com.formationds.security.AuthenticationToken;
import com.google.common.base.Preconditions;
import com.google.gson.reflect.TypeToken;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.uri.UriBuilderImpl;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.crypto.SecretKey;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import java.lang.reflect.Type;
import java.net.URI;
import java.util.List;
import java.util.Optional;

/**
 * @author ptinius
 */
// The v08 implementation is nearly identical to
// the original v07 api version, with the exception
// of the volume creation using the ExternalModelConverter
// and the API paths.  This is intended to be loaded behind a
// a feature toggle and work side-by-side with existing
// v07 api for now, with the expectation that it will soon
// completely replace the v07 version.
public class OMConfigServiceRestClientv08Impl implements OMConfigServiceClient {

    private static final Logger logger =
        LoggerFactory.getLogger( OMConfigServiceRestClientv08Impl.class);

    private static final String DEF_PROTOCOL = "http";
    private static final String DEF_API_PATH = "/fds/config/v08";
    private static final int    DEF_OM_PORT  = 7777;

    private static final String authUrl    = "/auth";
    private static final String tokenUrl   = "/token";
    private static final String volumesUrl = "/volumes";

    private static final String FDS_AUTH_HEADER = "FDS-Auth";

    private final String protocol;
    private final String host;
    private final Integer httpPort;
    private Client       client      = null;
    private WebResource  webResource = null;

    private final SecretKey key;

    /**
     * Create a rest client connection using the specified protocol and host ant port.
     *
     * @param protocol
     * @param host
     */
    public OMConfigServiceRestClientv08Impl(final SecretKey key,
                                            final String protocol,
                                            final String host,
                                            final Integer port) {
        super();

        this.key = key;
        this.protocol = protocol;
        this.host = host;
        this.httpPort = port;
    }

    private Client client() {

        if (client == null) {

            client = Client.create(new DefaultClientConfig());
        }

        return client;
    }

    private WebResource webResource() {

        if (webResource == null) {

            final URI uri =
                new UriBuilderImpl().scheme(protocol)
                                    .host(host)
                                    .port(httpPort != null ? httpPort : DEF_OM_PORT)
                                    .path(DEF_API_PATH)
                                    .build();

            webResource = client().resource(uri);
        }

        return webResource;
    }

    private String getToken(AuthenticationToken token) {
        return token.signature(key);
    }

    AuthenticatedUser login(String user, String password)
        throws OMConfigException {

        final ClientResponse response =
            webResource().path( authUrl )
                            .path(tokenUrl)
                            .queryParam("login", user)
                            .queryParam("password", password)
                            .type(MediaType.APPLICATION_JSON_TYPE)
                            .post(ClientResponse.class);
        isOk( response );

        return toAuthenticatedUser( response );
    }

    @Override
    public void createVolume(final AuthenticationToken token,
                             final String domain,
                             final String name,
                             final VolumeSettings volumeSettings,
                             final long tenantId)
        throws OMConfigException {

        Preconditions.checkNotNull(
            domain,
            "The specified argument for domain must not be null!" );
        Preconditions.checkNotNull(
            name,
            "The specified argument for volume name must not be null!" );
        Preconditions.checkNotNull(
            volumeSettings,
            "The specified argument for volume settings must not be null!" );
        Preconditions.checkNotNull(
            volumeSettings.getVolumeType(),
            "The specified argument within volume settings for volume type " +
            " must not be null!" );

        VolumeDescriptor vd = new VolumeDescriptor();
        vd.setName( name );
        vd.setTenantId( tenantId );
        vd.setPolicy( volumeSettings );

        // use the model converter to create the v08 representation of the volume
        final Volume volume = ExternalModelConverter.convertToExternalVolume( vd, null );

        // Set a default Qos Policy.  Without this, the OM handler will fail with
        // an exception indicating that the Qos Policy is not valid (null)
        volume.setQosPolicy( QosPolicy.defaultPolicy() );

        final ClientResponse response =
            webResource().path(volumesUrl)
                         .type(MediaType.APPLICATION_JSON_TYPE)
                         .header(FDS_AUTH_HEADER, getToken(token))
                         .post(ClientResponse.class,
                               ObjectModelHelper.toJSON(volume));

        isOk( response );

    }

    @Override
    public List<VolumeDescriptor> listVolumes(AuthenticationToken token, final String notUsedDomainName)
        throws OMConfigException {

        final ClientResponse response =
            webResource().path( volumesUrl )
                         .type( MediaType.APPLICATION_JSON_TYPE )
                         .header( FDS_AUTH_HEADER, getToken(token) )
                         .get( ClientResponse.class );
        isOk( response );

        return toListVolumeDescriptor( response );

    }

    @Override
    public long getVolumeId(AuthenticationToken token, final String notUsedDomainName, final String volumeName)
        throws OMConfigException {

        try {
            VolumeDescriptor volumeDescriptor = statVolume(token,
                                                           notUsedDomainName,
                                                           volumeName);
            if (volumeDescriptor != null) {
                return volumeDescriptor.getVolId();
            } else {
                return 0;
            }
        } catch (OMConfigException oce) {
            // volume does not exist.
            logger.debug(oce.getMessage());
            return 0;
        }
    }

    @Override
    public void deleteVolume(AuthenticationToken token, final String domainName, final String volumeName)
        throws OMConfigException {

        // rest call here is going to get all volumes and search for the requested one.  If you
        // can acess the volume id outside of this and call deleteVolume directly with that id,
        // it will be much more efficient.
        VolumeDescriptor vd = statVolume(token, domainName, volumeName);

        deleteVolume(token, vd.getVolId() );
    }

    public void deleteVolume(AuthenticationToken token, final Long volumeId)
        throws OMConfigException {

        final ClientResponse response =
            webResource().path( volumesUrl )
                         .path( Long.toString( volumeId ) )
                         .type( MediaType.APPLICATION_JSON_TYPE )
                         .header( FDS_AUTH_HEADER, getToken(token) )
                         .delete( ClientResponse.class );
        isOk( response );

    }

    @Override
    public VolumeDescriptor statVolume(AuthenticationToken token, final String domainName, final String volumeName)
        throws OMConfigException {

        final List<VolumeDescriptor> volumes = listVolumes(token, "not used!" );

        final Optional<VolumeDescriptor> optional =
            volumes.stream()
                   .filter( ( v ) -> v.getName()
                                      .equalsIgnoreCase( volumeName ) )
                   .findFirst();

        if( optional.isPresent() ) {

            return optional.get();

        }

        // or is not accessible by the user associated with the request
        logger.debug( "The specified volume ({}) does not exist or is not accessible by userid {}",
                      volumeName +
                      token.getUserId() );

        // maintain consistency with previous implementation of XDI ConfigurationApi
        // and return null instead of throwing exception
        return null;
    }

    private void isOk( final ClientResponse response ) throws OMConfigException {

        String json = response.getEntity( String.class );
        logger.debug( "ISOK::RESPONSE::{}::{}", response.getStatusInfo(), json );

        if( response.getStatusInfo().getStatusCode() != Response.Status.OK.getStatusCode() ) {

            logger.error( "ISOK::RESPONSE::FAILED::{}::{} ", response, json );

            JSONObject respObj = new JSONObject(json);
            String type = respObj.getString( "type" );
            String message = respObj.getString( "message" );
            ErrorCode errorCode = ErrorCode.INTERNAL_SERVER_ERROR;

            if ( type.endsWith("ApiException") ) {
                String respErrorCode = respObj.getString( "error_code" );
                errorCode = ErrorCode.valueOf( respErrorCode );
            }
            throw new OMConfigException( errorCode, "%s: %s", response.getStatusInfo(), message );

        }
    }

    private AuthenticatedUser toAuthenticatedUser( final ClientResponse response ) {

        final Type type = new TypeToken<AuthenticatedUser>() {}.getType();
        final AuthenticatedUser authUser =
            ObjectModelHelper.toObject( response.getEntity( String.class ),
                                        type );
        return authUser;
    }

    private long toLong( final ClientResponse response  ) {

        return Long.valueOf( response.getEntity( String.class ) );

    }

    private List<VolumeDescriptor> toListVolumeDescriptor ( final ClientResponse response ) {

        final Type type = new TypeToken<List<VolumeDescriptor>>() {}.getType();

        return ObjectModelHelper.toObject( response.getEntity( String.class ),
                                           type );
    }

}
