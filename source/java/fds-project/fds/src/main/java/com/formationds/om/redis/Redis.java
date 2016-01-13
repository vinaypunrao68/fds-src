/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.redis;

import FDS_ProtocolInterface.FDSP_AnnounceDiskCapability;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.platform.svclayer.SvcLayerSerializationProvider;
import com.formationds.platform.svclayer.SvcLayerSerializer;
import com.formationds.protocol.IScsiTarget;
import com.formationds.protocol.NfsOption;
import com.formationds.protocol.pm.types.NodeInfo;
import com.formationds.protocol.svc.types.FDSP_MgrIdType;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import com.formationds.protocol.svc.types.SvcInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import redis.clients.jedis.Jedis;
import redis.clients.jedis.JedisPool;
import redis.clients.jedis.JedisPoolConfig;
import redis.clients.jedis.Protocol;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * @author ptinius
 */
public class Redis
{
    private static final Logger logger = LoggerFactory.getLogger( Redis.class );

    private final JedisPool pool;

    public Redis( ) { this( Protocol.DEFAULT_HOST, Protocol.DEFAULT_PORT ); }
    public Redis( final String host ) { this( host, Protocol.DEFAULT_PORT ); }
    public Redis( final int port ) { this( Protocol.DEFAULT_HOST, port ); }
    public Redis( final String host, final int port )
    {
        pool = new JedisPool( new JedisPoolConfig(), host, port );
    }

    public List<SvcInfo> getPMSvcInfos()
    {
        final List<SvcInfo> svcs = new ArrayList<>( );

        svcs.addAll( getSvcInfos().stream()
                                  .filter( ( s ) -> s.getSvc_type() == FDSP_MgrIdType.FDSP_PLATFORM )
                                  .collect( Collectors.toList() ) );

        return svcs;
    }

    public List<SvcInfo> getDMSvcInfos()
    {
        final List<SvcInfo> svcs = new ArrayList<>( );

        svcs.addAll( getSvcInfos().stream()
                                  .filter( ( s ) -> s.getSvc_type() == FDSP_MgrIdType.FDSP_DATA_MGR )
                                  .collect( Collectors.toList() ) );

        return svcs;
    }

    public List<SvcInfo> getSMSvcInfos()
    {
        final List<SvcInfo> svcs = new ArrayList<>( );

        svcs.addAll( getSvcInfos().stream()
                                  .filter( ( s ) -> s.getSvc_type() == FDSP_MgrIdType.FDSP_STOR_MGR )
                                  .collect( Collectors.toList() ) );

        return svcs;
    }

    public List<SvcInfo> getAMSvcInfos()
    {
        final List<SvcInfo> svcs = new ArrayList<>( );

        svcs.addAll( getSvcInfos().stream()
                                  .filter( ( s ) -> s.getSvc_type() == FDSP_MgrIdType.FDSP_ACCESS_MGR )
                                  .collect( Collectors.toList() ) );

        return svcs;
    }

    public List<SvcInfo> getSvcInfos()
    {
        final List<SvcInfo> svcInfos = new ArrayList<>( );

        try ( Jedis jedis = pool.getResource( ) )
        {
            final Map<String, String> svcmap = jedis.hgetAll( "svcmap" );
            svcmap.values()
                  .stream()
                  .forEach( ( serialized ) ->
                          {
                              try
                              {
                                  final SvcInfo svc =
                                      SvcLayerSerializer.deserialize(
                                          SvcInfo.class,
                                          ByteBuffer.wrap( serialized.getBytes( ) ) );
                                  svcInfos.add( svc );
                              }
                              catch ( SvcLayerSerializationProvider.SerializationException e )
                              {
                                  logger.warn( "Encountered a de-serialization failure",
                                               e );

                                  logger.trace( "'[ " + serialized + " ]'" );
                              }
                          } );
        }

        return svcInfos;
    }

    public Optional<FDSP_Node_Info_Type> getNode( final long id )
    {
        if( id != 0 )
        {
            try ( Jedis jedis = pool.getResource( ) )
            {
                final String serialized = jedis.get( String.format( "node:%d", id ) );
                if ( serialized != null )
                {
                    return Optional.ofNullable(
                        SvcLayerSerializer.deserialize( FDSP_Node_Info_Type.class,
                                                        ByteBuffer.wrap( serialized.getBytes( ) ) ) );
                }
                else
                {
                    logger.warn( "The specified node id {} was not found.", id );
                }
            }
        }

        return Optional.empty();
    }

    public List<FDSP_Node_Info_Type> getNodes( )
    {
        final List<FDSP_Node_Info_Type> nodeInfos = new ArrayList<>( );

        for( final Long id : getNodeIds( 0 ) )
        {
            if( id == 0 ) { continue; }

            final Optional<FDSP_Node_Info_Type> node = getNode( id );
            if( node.isPresent() )
            {
                nodeInfos.add( node.get() );
            }
        }

        return nodeInfos;
    }

    public Map<String, FDSP_AnnounceDiskCapability> getPMNodeCapacity( )
    {
        final Map<String,FDSP_AnnounceDiskCapability> nodeDiskCapacity = new HashMap<>( );

        try ( Jedis jedis = pool.getResource( ) )
        {
            Set<String> sets = jedis.keys( "*.fds.node.disk.capability" );
            sets.stream( )
                .forEach( ( p ) ->
                          {
                              final String serialized = jedis.get( p );
                              if ( serialized != null )
                              {
                                  final FDSP_AnnounceDiskCapability capacity =
                                      SvcLayerSerializer.deserialize(
                                          FDSP_AnnounceDiskCapability.class,
                                          ByteBuffer.wrap( serialized.getBytes( ) ) );
                                  final String uuidKey = p.replace( ".fds.node.disk.capability", "" );
                                  nodeDiskCapacity.put( uuidKey, capacity );
                              }
                          } );
        }

        return nodeDiskCapacity;
    }

    public List<VolumeDesc> listVolumes()
    {
        final List<VolumeDesc> volumes = new ArrayList<>( );

        for( final Long id : getVolumeIds( 0 ) )
        {
            if( id == 0 ) { continue; }

            final Optional<VolumeDesc> volume = getVolume( id );
            if( volume.isPresent() )
            {
                volumes.add( volume.get() );
            }
        }

        return volumes;
    }

    public Optional<VolumeDesc> getVolume( final long id )
    {
        if( id > 0 )
        {
            try ( Jedis jedis = pool.getResource( ) )
            {
                final Map<String, String> fields = jedis.hgetAll( String.format( "vol:%d", id ) );
                if ( fields != null && !fields.isEmpty() )
                {
                    final VolumeDesc desc = new VolumeDesc( fields );
                    final VolumeSettings settings = new VolumeSettings( desc.maxObjectSize(),
                                                                        desc.type(),
                                                                        desc.maxObjectSize(),
                                                                        desc.retention(),
                                                                        desc.mediaPolicy() );
                    if( desc.type().equals( VolumeType.ISCSI ) ||
                        desc.type().equals( VolumeType.NFS ) )
                    {
                        final Optional<String> optional = getVolumeSettings( desc.id() );
                        if( optional.isPresent() )
                        {
                            final String serialized = optional.get();
                            switch( desc.type() )
                            {
                                case ISCSI:
                                    settings.iscsiTarget =
                                        SvcLayerSerializer.deserialize(
                                            IScsiTarget.class,
                                            ByteBuffer.wrap( serialized.getBytes( ) ) );
                                    break;
                                case NFS:
                                    settings.nfsOptions =
                                        SvcLayerSerializer.deserialize(
                                            NfsOption.class,
                                            ByteBuffer.wrap( serialized.getBytes( ) ) );
                                    break;
                            }
                        }
                    }
                    desc.settings( settings );
                    return Optional.of( desc );
                }
                else
                {
                    logger.warn( "The specified volume id {} was not found.", id );
                }
            }
        }

        return Optional.empty();
    }

    public Size getDomainUsedCapacity( )
    {
        long used;
        try ( Jedis jedis = pool.getResource( ) )
        {
            final Map<String, String> byNodes = jedis.hgetAll( "used.capacity" );
            System.out.println( byNodes );
            used = byNodes.values().stream().mapToLong( Long::valueOf ).sum( );
        }

        return Size.of( used, SizeUnit.B );
    }

    public Size getPMNodeUsedCapacity( final long id )
    {
        long used = 0;
        try ( Jedis jedis = pool.getResource( ) )
        {
            final Map<String, String> byNodes = jedis.hgetAll( "used.capacity" );
            if( byNodes.containsKey( String.valueOf( id ) ) )
            {
                used = Long.valueOf( byNodes.get( String.valueOf( id ) ) );
            }
            else
            {
                logger.warn( "The specified node uuid [ {} hex:{} ] was not found.",
                             id, Long.toHexString( id ) );
                System.out.println( "The specified node uuid [ " + id + " hex: 0x" + Long.toHexString( id ) +  " ] was not found." );
            }
        }

        return Size.of( used, SizeUnit.GB );
    }

    protected Optional<String> getVolumeSettings( final BigInteger uuid )
    {
        try ( Jedis jedis = pool.getResource( ) )
        {
            final String serialized = jedis.get( String.format( "vol:%d:settings", uuid ) );
            if( serialized != null && serialized.length() > 0 )
            {
                return Optional.of( serialized );
            }
        }

        return Optional.empty();
    }

    protected List<Long> getVolumeIds( final long localDomainId )
    {
        final List<Long> volumeIds = new ArrayList<>( );

        try ( Jedis jedis = pool.getResource( ) )
        {
            final Set<String> set = jedis.smembers( String.format( "%d:volumes",
                                                                   localDomainId ) );
            set.stream().forEach( ( n ) -> volumeIds.add( Long.valueOf( n ) ) );
        }

        return volumeIds;
    }

    protected List<Long> getNodeIds( final long localDomainId )
    {
        final List<Long> nodeIds = new ArrayList<>( );

        try ( Jedis jedis = pool.getResource( ) )
        {
            final Set<String> set = jedis.smembers( String.format( "%d:cluster:nodes",
                                                                   localDomainId ) );
            set.stream().forEach( ( n ) -> nodeIds.add( Long.valueOf( n ) ) );
        }

        return nodeIds;
    }

    protected NodeInfo getPMNodeInfo( final String uuidKey )
    {
        final NodeInfo[] nodeInfo = { null };
        final String key = String.format( "%s.fds.node.info", uuidKey );
        try ( Jedis jedis = pool.getResource( ) )
        {
            Set<String> sets = jedis.keys( key );
            sets.stream( )
                .forEach( ( p ) ->
                          {
                              final String serialized = jedis.get( p );
                              if ( serialized != null )
                              {
                                  try
                                  {
                                      nodeInfo[ 0 ] =
                                          SvcLayerSerializer.deserialize(
                                              NodeInfo.class,
                                              ByteBuffer.wrap( serialized.getBytes( ) ) );

                                      System.out.println( "NODE::" + nodeInfo[ 0 ].toString() );
                                  }
                                  catch ( SvcLayerSerializationProvider.SerializationException e )
                                  {
                                      final String warning =
                                          String.format( "The specified key '%s' encountered a de-serialization failure", key );

                                      logger.warn( warning, e );
                                  }
                              }
                          } );
        }

        return nodeInfo[ 0 ];
    }
}
