package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_GetVolInfoReqType;
import FDS_ProtocolInterface.FDSP_ModifyVolType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.om.helper.SingletonXdi;
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
    private FDSP_ConfigPathReq.Iface client;
    private ConfigurationService.Iface configService;
    private Authorizer authorizer;
    private AuthenticationToken token;

    public SetVolumeQosParams(FDSP_ConfigPathReq.Iface legacyClient,
                              ConfigurationService.Iface configService,
                              Authorizer authorizer,
                              AuthenticationToken token) {

        this.client = legacyClient;
        this.configService = configService;
        this.authorizer = authorizer;
        this.token = token;

    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long uuid = Long.parseLong(requiredString(routeParameters, "uuid"));
        JSONObject jsonObject = new JSONObject(IOUtils.toString(request.getInputStream()));
        int minIops = jsonObject.getInt("sla");
        int priority = jsonObject.getInt("priority");
        int maxIops = jsonObject.getInt("limit");
        long commit_log_retention = jsonObject.getLong( "commit_log_retention" );
        
        FDSP_VolumeDescType volumeDescType = client.ListVolumes(new FDSP_MsgHdrType())
                .stream()
                .filter(v -> v.getVolUUID() == uuid)
                .findFirst()
                .orElseThrow(() -> new RuntimeException("No such volume"));

        String volumeName = volumeDescType.getVol_name();
        if ( !authorizer.hasAccess(token, volumeName)) {
            return new JsonResource(new JSONObject().put("message", "Invalid permissions"), HttpServletResponse.SC_UNAUTHORIZED);
        }

        FDSP_VolumeDescType volInfo = setVolumeQos(client, volumeName, minIops, priority, maxIops, commit_log_retention );
        VolumeDescriptor descriptor = configService.statVolume("", volumeName);
        
        JSONObject o =
            ListVolumes.toJsonObject(descriptor,
                                     volInfo,
                             // TODO figure out how to get the current usages!
                                     SingletonXdi.instance().api().statVolume( token,
//                                     configService.statVolume(
                                         "",
                                         volumeName) );
        return new JsonResource(o);
    }

    public static FDSP_VolumeDescType setVolumeQos(FDSP_ConfigPathReq.Iface client, String volumeName, int minIops, int priority, int maxIops, long logRetention ) throws org.apache.thrift.TException {
        FDSP_VolumeDescType volInfo = client.GetVolInfo(new FDSP_MsgHdrType(), new FDSP_GetVolInfoReqType(volumeName, 0));
        volInfo.setIops_min(minIops);
        volInfo.setRel_prio(priority);
        volInfo.setIops_max(maxIops);
        volInfo.setVolPolicyId(0);
        volInfo.setContCommitlogRetention( logRetention );
        client.ModifyVol(new FDSP_MsgHdrType(), new FDSP_ModifyVolType(volInfo.getVol_name(),
                volInfo.getVolUUID(),
                volInfo));
        return volInfo;
    }
}
