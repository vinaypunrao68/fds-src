package com.formationds.om.webkit.rest.v07.volumes;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.FDSP_ModifyVolType;
import com.formationds.apis.FDSP_GetVolInfoReqType;
import com.formationds.om.webkit.rest.v07.volumes.ListVolumes;
import com.formationds.protocol.FDSP_VolumeDescType;
import com.formationds.protocol.FDSP_MediaPolicy;
import com.formationds.om.helper.MediaPolicyConverter;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;

import java.util.Map;

public class SetVolumeQosParams implements RequestHandler {
    private ConfigurationService.Iface configService;
    private Authorizer authorizer;
    private AuthenticationToken token;

    public SetVolumeQosParams(ConfigurationService.Iface configService,
                              Authorizer authorizer,
                              AuthenticationToken token) {

        this.configService = configService;
        this.authorizer = authorizer;
        this.token = token;

    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long uuid = Long.parseLong(requiredString(routeParameters, "uuid"));
        JSONObject jsonObject = new JSONObject(IOUtils.toString(request.getInputStream()));
        long assuredIops = jsonObject.getLong("sla");
        int priority = jsonObject.getInt("priority");
        long throttleIops = jsonObject.getLong("limit");
        long commit_log_retention = jsonObject.getLong( "commit_log_retention" );
        String mediaPolicyS = jsonObject.getString( "mediaPolicy" );
        MediaPolicy mediaPolicy = (mediaPolicyS != null && !mediaPolicyS.isEmpty() ?
                                   MediaPolicy.valueOf( mediaPolicyS ) :
                                   MediaPolicy.HDD_ONLY);

        FDSP_VolumeDescType volumeDescType = configService.ListVolumes(0)
                .stream()
                .filter(v -> v.getVolUUID() == uuid)
                .findFirst()
                .orElseThrow(() -> new RuntimeException("No such volume"));

        String volumeName = volumeDescType.getVol_name();
        if (!authorizer.ownsVolume(token, volumeName)) {
            return new JsonResource(new JSONObject().put("message", "Invalid permissions"), HttpServletResponse.SC_UNAUTHORIZED);
        }

        FDSP_VolumeDescType volInfo = setVolumeQos(configService, volumeName, assuredIops, priority, throttleIops, commit_log_retention, mediaPolicy );
        VolumeDescriptor descriptor = configService.statVolume("", volumeName);

        JSONObject o =
            ListVolumes.toJsonObject( descriptor,
                                      volInfo,
                                      // TODO figure out how to get the current usages!
                                      SingletonAmAPI.instance( )
                                                    .api( )
                                                    .volumeStatus( "",
                                                                   volumeName ) );
        return new JsonResource(o);
    }

    public static FDSP_VolumeDescType setVolumeQos(ConfigurationService.Iface configService, String volumeName, long assuredIops, int priority, long throttleIops, long logRetention, MediaPolicy mediaPolicy ) throws org.apache.thrift.TException {

    	// converting the com.formationds.api.MediaPolicy to the FDSP version
    	FDSP_MediaPolicy fdspMediaPolicy = MediaPolicyConverter.convertToFDSPMediaPolicy( mediaPolicy );
    	
    	FDSP_VolumeDescType volInfo = configService.GetVolInfo(new FDSP_GetVolInfoReqType(volumeName, 0));
        volInfo.setIops_assured(assuredIops);
        volInfo.setRel_prio(priority);
        volInfo.setIops_throttle(throttleIops);
        volInfo.setVolPolicyId(0);
        volInfo.setMediaPolicy(fdspMediaPolicy);
        volInfo.setContCommitlogRetention( logRetention );

        configService.ModifyVol(new FDSP_ModifyVolType(volInfo.getVol_name(),
                volInfo.getVolUUID(),
                volInfo));
        return volInfo;
    }
}
