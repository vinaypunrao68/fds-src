/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.om.rest.snapshot;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.formationds.commons.model.ObjectFactory;
import com.formationds.commons.model.Snapshot;
import com.formationds.commons.model.Status;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.rest.OMRestBase;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationServiceCache;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class ListSnapshotsByVolumeId
  extends OMRestBase
{
  private static final Logger LOG =
    Logger.getLogger( ListSnapshotsByVolumeId.class );

  private static final String REQ_PARAM_VOLUME_ID = "volumeId";

  /**
   * @param config the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  public ListSnapshotsByVolumeId( final ConfigurationServiceCache config )
  {
    super( config );
  }

  /**
   * @param request the {@link Request}
   * @param routeParameters the {@link Map} of route parameters
   *
   * @return Returns the {@link Resource}
   *
   * @throws Exception any unhandled error
   */
  @Override
  public Resource handle( final Request request,
                          final Map<String, String> routeParameters )
    throws Exception
  {
    final ObjectMapper mapper = new ObjectMapper();
    final List<Snapshot> snapshots = new ArrayList<>( );

    final long volumeId = requiredLong( routeParameters,
                                        REQ_PARAM_VOLUME_ID );
    if( FdsFeatureToggles.USE_CANNED.isActive() )
    {
      for( int i = 1; i <= 10 ; i++ )
      {
        final Snapshot mSnapshot = ObjectFactory.createSnapshot();

        mSnapshot.setId( i );
        mSnapshot.setName( String.format( "by volume policy name %d",
                                          volumeId ) );
        mSnapshot.setVolumeId( volumeId );
        mSnapshot.setCreation( new Date() );

        snapshots.add( mSnapshot );
      }
    }
    else
    {
      final List<com.formationds.apis.Snapshot> _snapshots =
        getConfigurationServiceCache().listSnapshots(
          requiredLong( routeParameters, REQ_PARAM_VOLUME_ID ) );
      if( _snapshots == null || _snapshots.isEmpty() )
      {
        final Status status = ObjectFactory.createStatus();
        status.setStatus( HttpResponseStatus.NO_CONTENT.reasonPhrase() );
        status.setCode( HttpResponseStatus.NO_CONTENT.code() );

        return new JsonResource( new JSONObject( mapper.writeValueAsString( status ) ) );
      }

      for( final com.formationds.apis.Snapshot snapshot : _snapshots )
      {
        final Snapshot mSnapshot = ObjectFactory.createSnapshot();

        mSnapshot.setId( snapshot.getSnapshotId() );
        mSnapshot.setName( snapshot.getSnapshotName() );
        mSnapshot.setVolumeId( snapshot.getVolumeId() );
        mSnapshot.setCreation( new Date( snapshot.getCreationTimestamp() ) );

        snapshots.add( mSnapshot );
      }
    }

    return new JsonResource( new JSONArray( mapper.writeValueAsString( snapshots ) ) );
  }
}
