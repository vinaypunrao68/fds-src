/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.SizeUnit;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.apache.commons.io.IOUtils;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

public class CreateVolume
  implements RequestHandler {
  private static final transient Logger logger =
    LoggerFactory.getLogger( CreateVolume.class );

  // TODO pull these values form the platform.conf file.
  private static final String URL = "http://localhost:7777/api/stats";
  private static final String METHOD = "POST";
  private static final Long DURATION = TimeUnit.MINUTES.toSeconds( 2 );
  private static final Long FREQUENCY = TimeUnit.MINUTES.toSeconds( 1 );

  private final Xdi xdi;
  private final FDSP_ConfigPathReq.Iface legacyConfigPath;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public CreateVolume( final Xdi xdi,
                       final FDSP_ConfigPathReq.Iface legacyConfigPath,
                       final ConfigurationService.Iface configApi,
                       final AuthenticationToken token ) {
    this.xdi = xdi;
    this.legacyConfigPath = legacyConfigPath;
    this.configApi = configApi;
    this.token = token;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {
    long volumeId;
    final String domainName = "";
      final int unused_argument = 0;

      String source = IOUtils.toString( request.getInputStream() );
      JSONObject o = new JSONObject( source );
    String name = o.getString( "name" );
    int priority = o.getInt( "priority" );
    int sla = o.getInt( "sla" );
    int limit = o.getInt( "limit" );
    JSONObject connector = o.getJSONObject( "data_connector" );
    String type = connector.getString( "type" );
    if( "block".equals( type.toLowerCase() ) ) {
      JSONObject attributes = connector.getJSONObject( "attributes" );
      int sizeUnits = attributes.getInt( "size" );
      long sizeInBytes = SizeUnit.valueOf( attributes.getString( "unit" ) )
                                 .totalBytes( sizeUnits );
      VolumeSettings volumeSettings = new VolumeSettings( 1024 * 4, VolumeType.BLOCK, sizeInBytes );
      volumeId = xdi.createVolume( token, domainName, name, volumeSettings );
    } else {
      volumeId = xdi.createVolume( token,
                                   domainName,
                                   name,
                                   new VolumeSettings( 1024 * 1024 * 2,
                                                       VolumeType.OBJECT,
                                                       0 ) );
    }

    Thread.sleep( 200 );
    SetVolumeQosParams.setVolumeQos( legacyConfigPath, name, sla, priority, limit );

    final Volume volume =
      new VolumeBuilder().withId( String.valueOf( volumeId ) )
                         .build();

    /**
     * there has to be a better way!
     *
     * If there are
     */
    final Map<Integer, List<String>> streamMap = new HashMap<>();
    configApi.getStreamRegistrations( unused_argument )
           .stream()
           .forEach( ( stream ) -> {
               if( stream.getVolume_names()
                         .contains( name ) ) {
                   final List<String> volumeNames = stream.getVolume_names();

                   streamMap.put( stream.getId(), volumeNames );
                   try {
                       configApi.deregisterStream( stream.getId() );
                   } catch( TException e ) {
                       logger.error( "Failed to de-register stream id " +
                                         stream.getId() +
                                         " reason: " + e.getMessage() );
                       logger.trace( "Failed to de-register stream id " +
                                         stream.getId(),
                                     e );
                   }
               }
           } );

    streamMap.forEach( ( key, list ) -> {
    logger.trace( "registering volume {} for metadata streaming...", name );
        try {
            configApi.registerStream( URL,
                                      METHOD,
                                      list,
                                      FREQUENCY.intValue(),
                                      DURATION.intValue() );
        } catch( TException e ) {
            logger.error( "Failed to re-register volumes " +
                          list + " reason: " + e.getMessage() );
            logger.trace( "Failed to re-register volumes " +
                          list , e );
        }
    } );

    return new JsonResource( new JSONObject( volume ) );
  }
}

