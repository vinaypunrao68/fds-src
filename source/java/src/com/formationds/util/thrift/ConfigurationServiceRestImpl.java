/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.util.thrift;


import com.formationds.apis.ConfigurationService;
import com.formationds.apis.Snapshot;
import com.formationds.apis.SnapshotPolicy;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.commons.model.AuthenticatedUser;
import com.formationds.commons.model.Connector;
import com.formationds.commons.model.ConnectorAttributes;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.ConnectorAttributesBuilder;
import com.formationds.commons.model.builder.ConnectorBuilder;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.ConnectorType;
import com.formationds.streaming.StreamingRegistrationMsg;
import com.formationds.util.SizeUnit;
import com.google.common.base.Preconditions;
import com.google.common.net.HostAndPort;
import com.google.gson.reflect.TypeToken;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.uri.UriBuilderImpl;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.ws.rs.core.MediaType;
import java.lang.reflect.Type;
import java.net.URI;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

/**
 * @author ptinius
 */
public class ConfigurationServiceRestImpl
        implements ConfigurationService.Iface {

    private static final Logger logger =
        LoggerFactory.getLogger( ConfigurationServiceRestImpl.class );

    private static final String DEF_PROTOCOL = "http";
    private static final String DEF_API_PATH = "/api";
    private static final int DEF_OM_PORT = 7777;

    private final HostAndPort host;
    private final String protocol;

    private static final String authUrl = "/auth";
    private static final String configUrl = "/config";
    private static final String tokenUrl = "/token";
    private static final String volumesUrl = "/volumes";

    private static final String FDS_AUTH_HEADER = "FDS-Auth";
    private String token = null;

    public ConfigurationServiceRestImpl( final String host ) {

        this( DEF_PROTOCOL, HostAndPort.fromParts( host, DEF_OM_PORT ) );

    }

    public ConfigurationServiceRestImpl( final String protocol,
                                         final HostAndPort host ) {
        super();

        this.host = host;
        this.protocol = protocol;

    }

    private Client client = null;
    private WebResource webResource = null;

    private Client client( ) {

        if( client == null ) {

            client = Client.create( new DefaultClientConfig() );

        }

        return client;

    }

    private WebResource webResource( ) {

        if( webResource == null ) {

            final URI uri =
                new UriBuilderImpl().scheme( protocol )
                                    .host( host.getHostText() )
                                    .port( host.getPortOrDefault( DEF_OM_PORT ) )
                                    .path( DEF_API_PATH )
                                    .build();

            webResource = client().resource( uri );
        }

        return webResource;
    }

    /**
     * @param token the {@link String} representing the token
     */
    private void setToken( final String token ) {
        this.token = token;
    }

    /**
     * @return Returns {@link String} representing the token
     */
    private String getToken() {
        return token;
    }

    void login()
        throws TException {

        final ClientResponse response =
            webResource().path( authUrl )
                            .path( tokenUrl )
                            .queryParam( "login", "admin" )
                            .queryParam( "password", "admin" )
                            .type( MediaType.APPLICATION_JSON_TYPE )
                            .post( ClientResponse.class );
        isOk( response );

        toAuthenticatedUser( response );
    }

    private void isOk( final ClientResponse response )
        throws TException {
        logger.error( "ISOK::RESPONSE::" + response.toString() );
        if( !response.getClientResponseStatus()
                     .equals( ClientResponse.Status.OK ) ) {

            logger.error( response.toString() );

            throw new TException( response.toString() );

        }
    }

    private void toAuthenticatedUser( final ClientResponse response ) {

        final Type type = new TypeToken<AuthenticatedUser>() {}.getType();
        final AuthenticatedUser authUser =
            ObjectModelHelper.toObject( response.getEntity( String.class ),
                                        type );
        if( authUser != null ) {

            setToken( authUser.getToken() );

        }
    }

    private long toLong( final ClientResponse response  ) {

        return Long.valueOf( response.getEntity( String.class ) );

    }

    private List<VolumeDescriptor> toListVolumeDescriptor (
        final ClientResponse response ) {

        final Type type = new TypeToken<List<VolumeDescriptor>>() {}.getType();

        return ObjectModelHelper.toObject( response.getEntity( String.class ),
                                           type );
    }

    // STAND-A-LONE TEST
    public static void main( String[] args )
        throws TException {

        final ConfigurationServiceRestImpl impl =
            new ConfigurationServiceRestImpl( "10.211.55.9" );

        final String domainName = "";
        final String objectVolumeName = "TestVolume_object";
        final String blockVolumeName = "TestVolume_block";
        final VolumeSettings blockSettings = new VolumeSettings( );
        blockSettings.setVolumeType( VolumeType.BLOCK );
        blockSettings.setBlockDeviceSizeInBytes( Integer.MAX_VALUE );
        blockSettings.setBlockDeviceSizeInBytesIsSet( true );
        blockSettings.setContCommitlogRetention( Instant.now().getEpochSecond() );
        blockSettings.setContCommitlogRetentionIsSet( true );
        blockSettings.setMaxObjectSizeInBytesIsSet( false );

        final VolumeSettings objectSettings = new VolumeSettings( );
        objectSettings.setVolumeType( VolumeType.OBJECT );
        objectSettings.setBlockDeviceSizeInBytesIsSet( false );
        objectSettings.setContCommitlogRetention( Instant.now().getEpochSecond() );
        objectSettings.setContCommitlogRetentionIsSet( true );
        objectSettings.setMaxObjectSizeInBytes( Integer.MAX_VALUE );
        objectSettings.setMaxObjectSizeInBytesIsSet( true );

        impl.login();
        impl.createVolume( domainName, blockVolumeName, blockSettings, 0 );
        impl.createVolume( domainName, objectVolumeName, objectSettings, 0 );
        System.out.println( impl.listVolumes( domainName ) );
        System.out.println( "VolumeId::" + impl.getVolumeId( blockVolumeName ) );
        System.out.println( "VolumeStats::" + impl.statVolume( domainName,
                                                               objectVolumeName ) );

    }

    @Override
    public void createVolume( final String domain,
                              final String name,
                              final VolumeSettings volumeSettings,
                              final long tenantId )
        throws  TException {

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

        Connector connector;
        switch( volumeSettings.getVolumeType() ) {
            case OBJECT: // Object volume type
                connector =
                    new ConnectorBuilder().withType( ConnectorType.OBJECT )
                                          .withApi( "S3, Swift" )
                                          .build();
                break;
            case BLOCK: // Block volume type
                Preconditions.checkArgument(
                    volumeSettings.getBlockDeviceSizeInBytes() > 0,
                    "The specified block device %s size is invalid",
                    name );

                final ConnectorAttributes attrs =
                    new ConnectorAttributesBuilder()
                        .withSize( volumeSettings.getBlockDeviceSizeInBytes() )
                        .withUnit( SizeUnit.B )
                        .build();
                connector =
                    new ConnectorBuilder().withType( ConnectorType.BLOCK )
                                          .withAttributes( attrs )
                                          .build();
                break;
            default:
                throw new TException(
                    "The specified volume type " +
                    volumeSettings.getVolumeType().getValue() +
                    " is invalid." );
        }

        final Volume volume =
            new VolumeBuilder().withName( name )
                               .withLimit( 0 )          // default for now
                               .withPriority( 10 )      // default for now
                               .withSla( 0 )            // default for now
                               .withTenantId( tenantId )
                               .withCommitLogRetention(
                                   volumeSettings.getContCommitlogRetention() )
                               .withData_connector( connector )
                               .build();

        final ClientResponse response =
            webResource().path( configUrl )
                         .path( volumesUrl )
                         .type( MediaType.APPLICATION_JSON_TYPE )
                         .header( FDS_AUTH_HEADER, getToken() )
                         .post( ClientResponse.class,
                                ObjectModelHelper.toJSON( volume ) );

        isOk( response );

    }


    @Override
    public List<VolumeDescriptor> listVolumes( final String notUsedDomainName )
        throws  TException {

        final ClientResponse response =
            webResource().path( configUrl )
                         .path( volumesUrl )
                         .type( MediaType.APPLICATION_JSON_TYPE )
                         .header( FDS_AUTH_HEADER, getToken() )
                         .get( ClientResponse.class );
        isOk( response );

        return toListVolumeDescriptor( response );

    }

    @Override
    public long getVolumeId( final String volumeName )
        throws  TException {

        final List<VolumeDescriptor> volumes = listVolumes( "not used!" );

        final Optional<VolumeDescriptor> optional =
            volumes.stream()
                   .filter( ( v ) -> v.getName()
                                      .equalsIgnoreCase( volumeName ) )
                   .findFirst();

        if( optional.isPresent() ) {

            return optional.get().getVolId();

        }

        throw new TException( "The specific volume ( " +
                              volumeName +
                              " ) does not exists." );
    }

    @Override
    public void deleteVolume( final String domainName, final String volumeName )
        throws  TException {

        final ClientResponse response =
            webResource().path( configUrl )
                         .path( volumesUrl )
                         .queryParam( "name", volumeName )
                         .type( MediaType.APPLICATION_JSON_TYPE )
                         .header( FDS_AUTH_HEADER, getToken() )
                         .post( ClientResponse.class );
        isOk( response );

    }

    @Override
    public VolumeDescriptor statVolume( final String domainName, final String volumeName )
        throws  TException {

        final List<VolumeDescriptor> volumes = listVolumes( "not used!" );

        final Optional<VolumeDescriptor> optional =
            volumes.stream()
                   .filter( ( v ) -> v.getName()
                                      .equalsIgnoreCase( volumeName ) )
                   .findFirst();

        if( optional.isPresent() ) {

            return optional.get();

        }

        throw new TException( "The specific volume ( " +
                                  volumeName +
                                  " ) does not exists." );

    }

    /* TODO Finish non-XDI implementations -- UNSUPPORTED */
    @Override
    public long createTenant( final String s )
        throws TException {

        throw new TException(
            "unsupported configuration service request -- createTenant" );

    }

    @Override
    public List<Tenant> listTenants( final int i )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- listTenants" );
    }

    @Override
    public long createUser( final String s, final String s1, final String s2, final boolean b )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- createUser" );

    }

    @Override
    public void assignUserToTenant( final long l, final long l1 )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- assignUserToTenant" );

    }

    @Override
    public void revokeUserFromTenant( final long l, final long l1 )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- revokeUserFromTenant" );

    }

    @Override
    public List<User> allUsers( final long l )
        throws TException {

        throw new TException(
            "unsupported configuration service request -- allUsers" );

    }

    @Override
    public List<User> listUsersForTenant( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- listUsersForTenant" );

    }

    @Override
    public void updateUser( final long l, final String s, final String s1, final String s2, final boolean b )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- updateUser" );

    }

    @Override
    public long configurationVersion( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- configurationVersion" );

    }


    @Override
    public String getVolumeName( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- getVolumeName" );
    }

    @Override
    public int registerStream( final String s, final String s1, final List<String> list, final int i, final int i1 )
        throws TException {

        throw new TException(
            "unsupported configuration service request -- registerStream" );
    }

    @Override
    public List<StreamingRegistrationMsg> getStreamRegistrations( final int i )
        throws TException {

        throw new TException(
            "unsupported configuration service request -- getStreamRegistration" );
    }

    @Override
    public void deregisterStream( final int i )
        throws TException {

        throw new TException(
            "unsupported configuration service request -- deregisterStream" );
    }

    @Override
    public long createSnapshotPolicy( final SnapshotPolicy snapshotPolicy )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- createSnapshotPolicy" );
    }

    @Override
    public List<SnapshotPolicy> listSnapshotPolicies( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- listSnapshotPolicies" );
    }

    @Override
    public void deleteSnapshotPolicy( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- deleteSnapshotPolicy" );
    }

    @Override
    public void attachSnapshotPolicy( final long l, final long l1 )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- attachSnapshotPolicy" );
    }

    @Override
    public List<SnapshotPolicy> listSnapshotPoliciesForVolume( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- listSnapshotPoliciesFroVolume" );
    }

    @Override
    public void detachSnapshotPolicy( final long l, final long l1 )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- detachSnapshotPolicy" );
    }

    @Override
    public List<Long> listVolumesForSnapshotPolicy( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- listVolumesForSnapshotPolicy" );
    }

    @Override
    public void createSnapshot( final long l, final String s, final long l1, final long l2 )
        throws  TException {
        throw new TException(
            "unsupported configuration service request -- createSnapshot" );
    }

    @Override
    public List<Snapshot> listSnapshots( final long l )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- listSnapshots" );
    }

    @Override
    public void restoreClone( final long l, final long l1 )
        throws  TException {

        throw new TException(
            "unsupported configuration service request -- restoreClone" );
    }

    @Override
    public long cloneVolume( final long l, final long l1, final String s, final long l2 )
        throws TException {

        throw new TException(
            "unsupported configuration service request -- cloneVolume" );
    }
}
