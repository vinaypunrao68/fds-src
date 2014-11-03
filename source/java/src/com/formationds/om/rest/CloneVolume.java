/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import com.google.gson.GsonBuilder;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

public class CloneVolume
  implements RequestHandler {

  private static final String REQ_PARAM_VOLUME_ID = "volumeId";
  private static final String REQ_PARAM_CLONE_VOLUME_NAME = "cloneVolumeName";

  private ConfigurationApi config;
  private FDSP_ConfigPathReq.Iface legacyConfigPath;

  public CloneVolume( final ConfigurationApi config,
                      FDSP_ConfigPathReq.Iface legacyConfigPath ) {
    this.config = config;
    this.legacyConfigPath = legacyConfigPath;
  }

  @Override
  public Resource handle( final Request request,
                          final Map<String, String> routeParameters )
    throws Exception {
    long clonedVolumeId;
    try( final Reader reader =
           new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {

      final Volume volume = new GsonBuilder().create()
                                             .fromJson( reader,
                                                        Volume.class );
      final String name = requiredString( routeParameters,
                                          REQ_PARAM_CLONE_VOLUME_NAME );
      clonedVolumeId = config.cloneVolume(
        requiredLong( routeParameters, REQ_PARAM_VOLUME_ID ),
        0L,                 // optional parameter so setting it to zero!
        name );

      Thread.sleep( 200 );
      SetVolumeQosParams.setVolumeQos( legacyConfigPath,
                                       name,
                                       ( int ) volume.getSla(),
                                       volume.getPriority(),
                                       ( int ) volume.getLimit() );
    }

    // TODO register to receive metadata, i.e. statistics
    // TODO audit and/or alert/event

    return new JsonResource(
      new JSONObject(
        new VolumeBuilder().withId( String.valueOf( clonedVolumeId ) )
                           .build() ) );
  }
}
