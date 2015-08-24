/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */
package com.formationds.tools.statgen;

import com.formationds.client.v08.model.*;
import com.google.common.base.Preconditions;
import com.google.gson.reflect.TypeToken;
import com.sun.jersey.api.client.ClientResponse;

import javax.ws.rs.core.MediaType;
import java.lang.reflect.Type;
import java.util.List;
import java.util.Objects;
import java.util.Optional;

/**
 */
// TODO: copied from fds project and refactored to remove some dependencies.
public class OMConfigServiceRestClient implements StatWriter, VolumeManager {

    // TODO: duplicates fds commons.model.AuthenticatedUser
    public static class AuthenticatedUser {
        private       long          userId;
        private final String        username;
        private final String        token;
        private       List<Feature> features;

        public AuthenticatedUser( long userId, String username, String token,
                                  List<Feature> features ) {
            this.userId = userId;
            this.username = username;
            this.token = token;
            this.features = features;
        }

        public AuthenticatedUser( String username, String token ) {
            this.username = username;
            this.token = token;
        }

        public String getUsername() {
            return username;
        }

        public String getToken() {
            return token;
        }

        public long getUserId() {
            return userId;
        }

        public List<Feature> getFeatures() {
            return features;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof AuthenticatedUser) ) { return false; }
            final AuthenticatedUser that = (AuthenticatedUser) o;
            return Objects.equals( userId, that.userId ) &&
                   Objects.equals( username, that.username ) &&
                   Objects.equals( token, that.token );
        }

        @Override
        public int hashCode() {
            return Objects.hash( userId, username, token );
        }

        @Override
        public String toString() {
            final StringBuilder sb = new StringBuilder( "AuthenticatedUser{" );
            sb.append( "userId=" ).append( userId );
            sb.append( ", username='" ).append( username ).append( '\'' );
            sb.append( ", token='" ).append( token ).append( '\'' );
            sb.append( ", features=" ).append( features );
            sb.append( '}' );
            return sb.toString();
        }
    }

    private static final String DEF_PROTOCOL = "http";
    private static final int    DEF_OM_PORT  = 7777;

    private static final String URL_PREFIX = "/fds/config/v08";

    //    private static final String authUrl    = "/auth";
    private static final String tokenUrl   = "/token";
    private static final String volumesUrl = "/volumes";
    private static final String tenantUrl  = "/tenants";
    private static final String nodesUrl   = "/nodes";
    private static final String statsUrl   = "/stats";

    private static final String FDS_AUTH_HEADER = "FDS-Auth";
    private final RestClient restClient;

    private AuthenticatedUser authUser;

    /**
     * Create a rest client connection using the specified protocol and host ant port.
     *
     * @param host
     * @param port
     */
    public OMConfigServiceRestClient( final String host,
                                      final Integer port ) {
        this( DEF_PROTOCOL, host, port );
    }

    /**
     * Create a rest client connection using the specified protocol and host ant port.
     *
     * @param protocol
     * @param host
     * @param port
     */
    public OMConfigServiceRestClient( final String protocol,
                                      final String host,
                                      final Integer port ) {
        restClient = new RestClient( host, port, protocol, URL_PREFIX );
    }

    private String getToken( AuthenticatedUser token ) {
        return token.getToken();
    }

    public void login( String user, String password ) {
        authUser = doLogin( user, password );
    }

    private AuthenticatedUser doLogin( String user, String password )
        throws RestClient.ClientException {

        final ClientResponse response =
            restClient.webResource().path( tokenUrl )
                      .queryParam( "login", user )
                      .queryParam( "password", password )
                      .type( MediaType.APPLICATION_JSON_TYPE )
                      .post( ClientResponse.class );
        restClient.isOk( response );

        return toAuthenticatedUser( user, response );
    }

    @Override
    public void createBlockVolume( final String domain,
                                   final String name,
                                   final Tenant tenant,
                                   final long capacity,
                                   final long blockSize,
                                   final SizeUnit unit ) throws RestClient.ClientException {
        createVolume( domain, name, tenant, new VolumeSettingsBlock( capacity, blockSize, unit ) );
    }

    @Override
    public void createBlockVolume( final String domain,
                                   final String name,
                                   final Tenant tenant,
                                   final Size capacity,
                                   final Size blockSize ) throws RestClient.ClientException {
        createVolume( domain, name, tenant, new VolumeSettingsBlock( capacity, blockSize ) );
    }

    @Override
    public void createObjectVolume( final String domain,
                                    final String name,
                                    final Tenant tenant,
                                    final long maxObjectSize,
                                    final SizeUnit unit ) throws RestClient.ClientException {
        createVolume( domain, name, tenant, new VolumeSettingsObject( maxObjectSize, unit ) );
    }

    @Override
    public void createObjectVolume( final String domain,
                                    final String name,
                                    final Tenant tenant,
                                    final Size maxObjectSize ) throws RestClient.ClientException {
        createVolume( domain, name, tenant, new VolumeSettingsObject( maxObjectSize ) );
    }

    @Override
    public void createVolume( final String domain,
                              final String name,
                              final Tenant tenant,
                              final VolumeSettings settings ) throws RestClient.ClientException {

        Preconditions.checkNotNull( domain,
                                    "The specified argument for domain must not be null!" );
        Preconditions.checkNotNull( name,
                                    "The specified argument for volume name must not be null!" );

        final Volume volume =
            new Volume.Builder( name )
                .tenant( tenant )
                .settings( settings )
                .mediaPolicy( MediaPolicy.HYBRID )
                .dataProtectionPolicy( 0, TimeUnit.HOURS, new SnapshotPolicy[0] )
                .accessPolicy( VolumeAccessPolicy.exclusiveWritePolicy() )
                .qosPolicy( QosPolicy.of( 5, 0, 0 ) )
                .application( "StatGenerator" )
                .create();

        createVolume( volume );
    }

    @Override
    public void createVolume( Volume volume ) throws RestClient.ClientException {
        final ClientResponse response =
            restClient.webResource().path( volumesUrl )
                      .type( MediaType.APPLICATION_JSON_TYPE )
                      .header( FDS_AUTH_HEADER, getToken( authUser ) )
                      .post( ClientResponse.class,
                             RestClient.GSON.toJSON( volume ) );

        restClient.isOk( response );
    }

    @Override
    public List<Volume> listVolumes() throws RestClient.ClientException {
        return listVolumes( "" );
    }

    @Override
    public List<Volume> listVolumes( final String notUsedDomainName ) throws RestClient.ClientException {

        final ClientResponse response =
            restClient.webResource().path( volumesUrl )
                      .type( MediaType.APPLICATION_JSON_TYPE )
                      .header( FDS_AUTH_HEADER, getToken( authUser ) )
                      .get( ClientResponse.class );
        restClient.isOk( response );

        return toListVolumeDescriptor( response );
    }

    @Override
    public long getVolumeId( final String notUsedDomainName,
                             final String volumeName ) throws RestClient.ClientException {

        try {
            Volume volumeDescriptor = statVolume( notUsedDomainName,
                                                  volumeName );
            if ( volumeDescriptor != null ) {
                return volumeDescriptor.getId();
            } else {
                return 0;
            }
        } catch ( RestClient.ClientException oce ) {
            // volume does not exist.
            RestClient.logger.debug( oce.getMessage() );
            return 0;
        }
    }

    @Override
    public void deleteVolume( final String domainName, final String volumeName )
        throws RestClient.ClientException {

        final ClientResponse response =
            restClient.webResource().path( volumesUrl )
                      .path( volumeName )
                      .type( MediaType.APPLICATION_JSON_TYPE )
                      .header( FDS_AUTH_HEADER, getToken( authUser ) )
                      .delete( ClientResponse.class );
        restClient.isOk( response );
    }

    @Override
    public Volume statVolume( final String domainName, final String volumeName )
        throws RestClient.ClientException {

        final List<Volume> volumes = listVolumes( "not used!" );

        final Optional<Volume> optional =
            volumes.stream()
                   .filter( ( v ) -> v.getName()
                                      .equalsIgnoreCase( volumeName ) )
                   .findFirst();

        if ( optional.isPresent() ) {

            return optional.get();
        }

        // or is not accessible by the user associated with the request
        RestClient.logger.debug( "The specified volume ( " +
                                 volumeName +
                                 " ) does not exist." );

        // TODO: remove
        if ( RestClient.logger.isTraceEnabled() ) {
            RestClient.logger.trace( "Volume " +
                                     volumeName +
                                     " either does not exist or is not accessible by userid: " +
                                     authUser.getUsername() );
        }

        // maintain consistency with previous implementation of XDI ConfigurationApi
        // and return null instead of throwing exception
        return null;
    }

    public Tenant getTenant( String name ) {
        return getTenants().stream()
                           .filter( tenant -> tenant.getName().equals( name ) )
                           .findFirst()
                           .orElse( null );
    }

    public List<Tenant> getTenants() {

        // todo: cache them
        final ClientResponse response =
            restClient.webResource().path( tenantUrl )
                      .type( MediaType.APPLICATION_JSON_TYPE )
                      .header( FDS_AUTH_HEADER, getToken( authUser ) )
                      .get( ClientResponse.class );
        restClient.isOk( response );

        return toListTenant( response );
    }

    public void sendStats( List<VolumeDatapoint> volumeDatapoints ) {
        final ClientResponse response =
            restClient.webResource().path( statsUrl )
                      .type( MediaType.APPLICATION_JSON_TYPE )
                      .header( FDS_AUTH_HEADER, getToken( authUser ) )
                      .post( ClientResponse.class,
                             RestClient.GSON.toJSON( volumeDatapoints ) );
        restClient.isOk( response );
    }

    private AuthenticatedUser toAuthenticatedUser( final String user, final ClientResponse response ) {

        final Type type = new TypeToken<AuthenticatedUser>() {}.getType();
        final AuthenticatedUser authenticatedUser =
            RestClient.GSON.toObject( response.getEntity( String.class ),
                                      type );
        return authenticatedUser;
    }

    private long toLong( final ClientResponse response ) {

        return Long.valueOf( response.getEntity( String.class ) );
    }

    private List<Volume> toListVolumeDescriptor( final ClientResponse response ) {

        final Type type = new TypeToken<List<Volume>>() {}.getType();

        return RestClient.GSON.toObject( response.getEntity( String.class ),
                                         type );
    }

    private List<Tenant> toListTenant( final ClientResponse response ) {

        final Type type = new TypeToken<List<Tenant>>() {}.getType();

        return RestClient.GSON.toObject( response.getEntity( String.class ),
                                         type );
    }
}
