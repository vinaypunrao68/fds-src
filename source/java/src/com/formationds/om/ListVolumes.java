package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_GetVolInfoReqType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.google.common.base.Joiner;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;

import java.util.List;
import java.util.Map;

public class ListVolumes implements RequestHandler {
    private FDSP_ConfigPathReq.Iface iface;

    public ListVolumes(FDSP_ConfigPathReq.Iface iface) {
        this.iface = iface;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        List<FDSP_VolumeDescType> volumes = iface.ListVolumes(msg);
        String[] descriptions = volumes.stream()
                .map(v -> {
                    try {
                        FDSP_VolumeDescType volInfo = iface.GetVolInfo(msg, new FDSP_GetVolInfoReqType(v.getVol_name(), v.getGlobDomainId()));
                        return new HalfFakeVolume(volInfo).toString();

                    } catch (TException e) {
                        throw new RuntimeException(e);
                    }
                })
                .toArray(i -> new String[i]);
        return new TextResource("[" + Joiner.on(",\n").join(descriptions) + "]");
    }

}
