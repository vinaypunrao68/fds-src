/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util.net.service;

import com.formationds.commons.util.NodeUtils;
import com.formationds.protocol.SvcUuid;
import com.formationds.protocol.svc.types.SvcInfo;
import com.google.common.base.Preconditions;

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

/**
 * @author ptinius
 */
public class ServiceMap
{
    private final ConcurrentHashMap<SvcUuid, List<SvcInfo>> svcmap =
        new ConcurrentHashMap<>( );

    /**
     * @param svcUuid the {@link SvcUuid} representing the service to get
     *
     * @return Returns {@link Optional} of {@link SvcInfo} or
     *         {@link Optional#empty()} if no matching are found
     */
    public Optional<SvcInfo> get( final SvcUuid svcUuid )
    {
        Preconditions.checkNotNull( svcUuid );
        final SvcUuid nodeUuid = NodeUtils.getNodeUUID( svcUuid );

        if( svcmap.containsKey( nodeUuid ) )
        {
            for( final SvcInfo svc : svcmap.get( nodeUuid ) )
            {
                if( svc.getSvc_id().getSvc_uuid().getSvc_uuid() ==
                    svcUuid.getSvc_uuid() )
                {
                    return Optional.of( svc );
                }
            }
        }

        return Optional.empty();
    }

    /**
     * @return Returns {@link List} of {@link SvcInfo} or
     *         empty {@link List} is no services exists within the
     *         service map.
     */
    public List<SvcInfo> get( )
    {
        final List<SvcInfo> _svcmap = new ArrayList<>( );

        final Enumeration<List<SvcInfo>> enumeration = svcmap.elements( );
        while( enumeration.hasMoreElements() )
        {
            _svcmap.addAll( enumeration.nextElement() );
        }

        return _svcmap;
    }

    /**
     * @param svcInfo the {@link SvcInfo} representing the updated details
     */
    public void put( final SvcInfo svcInfo )
    {
        Preconditions.checkNotNull( svcInfo );

        final SvcUuid nodeUuid =
            NodeUtils.getNodeUUID( svcInfo.getSvc_id().getSvc_uuid() );
        if( svcmap.containsKey( nodeUuid ) )
        {
            final List<SvcInfo> svcs = getNodeSvcs( nodeUuid );
            svcs.add( svcInfo );
        }
        else
        {
            final List<SvcInfo> svcs = new ArrayList<>( );
            svcs.add( svcInfo );
            svcmap.put( nodeUuid, svcs );
        }
    }

    /**
     * @param nodeUuid the {@link SvcUuid} representing the service uuid for the node
     *
     * @return Returns {@link List} of {@link SvcInfo} derived from the {@code svcUuid}
     */
    public List<SvcInfo> getNodeSvcs( final SvcUuid nodeUuid )
    {
        Preconditions.checkNotNull( nodeUuid );

        if( svcmap.containsKey( nodeUuid ) )
        {
            return svcmap.get( nodeUuid );
        }

        return new ArrayList<>( );
    }

    /**
     * @param svcInfo the {@link SvcInfo} representing the updated details
     *
     * @return Returns {@link List} of {@link SvcInfo}.
     */
    public List<SvcInfo> update( final SvcInfo svcInfo )
    {
        put( svcInfo );
        return get( );
    }

    /**
     * @return Returns {@code int} of the size of the service map
     */
    public int size()
    {
        return svcmap.size( );
    }
}
