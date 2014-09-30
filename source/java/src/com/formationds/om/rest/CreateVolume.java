package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
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
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class CreateVolume implements RequestHandler {
  private static final Logger LOG = Logger.getLogger(CreateVolume.class);

    private Xdi xdi;
    private FDSP_ConfigPathReq.Iface legacyConfigPath;
    private AuthenticationToken token;

    public CreateVolume(Xdi xdi, FDSP_ConfigPathReq.Iface legacyConfigPath, AuthenticationToken token) {
        this.xdi = xdi;
        this.legacyConfigPath = legacyConfigPath;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long volumeId;
      final String domainName = "";

        String source = IOUtils.toString(request.getInputStream());
        JSONObject o = new JSONObject(source);
        String name = o.getString("name");
        int priority = o.getInt("priority");
        int sla = o.getInt("sla");
        int limit = o.getInt("limit");
        JSONObject connector = o.getJSONObject("data_connector");
        String type = connector.getString("type");
        if ("block".equals(type.toLowerCase())) {
            JSONObject attributes = connector.getJSONObject("attributes");
            int sizeUnits = attributes.getInt("size");
            long sizeInBytes = SizeUnit.valueOf(attributes.getString("unit")).totalBytes(sizeUnits);
            VolumeSettings volumeSettings = new VolumeSettings(1024 * 4, VolumeType.BLOCK, sizeInBytes);
            volumeId = xdi.createVolume(token, domainName, name, volumeSettings);
        } else {
            volumeId = xdi.createVolume(token,
                                        domainName,
                                        name,
                                        new VolumeSettings( 1024 * 1024 * 2,
                                                            VolumeType.OBJECT,
                                                            0 ) );
        }

        Thread.sleep(200);
        SetVolumeQosParams.setVolumeQos(legacyConfigPath, name, sla, priority, limit);

        final Volume volume =
          new VolumeBuilder().withId( String.valueOf( volumeId ) )
                             .withLimit( limit )
                             .withSla( sla ).withPriority( priority )
                             .build();

        return new JsonResource( new JSONObject( volume ) );
    }
}

