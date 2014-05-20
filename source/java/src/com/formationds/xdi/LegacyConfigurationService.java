package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.apis.*;
import com.formationds.fdsp.ClientFactory;
import com.formationds.om.CreateVolume;
import org.apache.thrift.TException;
import org.joda.time.DateTime;

import java.util.List;
import java.util.stream.Collectors;

public class LegacyConfigurationService implements ConfigurationService.Iface {
    private final FDSP_ConfigPathReq.Iface client;

    public LegacyConfigurationService(String host, int port) {
        client = new ClientFactory().configPathClient(host, port);
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws ApiException, TException {
        new CreateVolume(client).createVolume(volumeName, 0, Integer.MAX_VALUE, 1);
    }

    @Override
    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
        client.DeleteVol(new FDSP_MsgHdrType(), new FDSP_DeleteVolType(volumeName, 0));
    }

    @Override
    public VolumeDescriptor statVolume(String domainName, String volumeName) throws ApiException, TException {
        FDSP_VolumeDescType volumeDescriptor = client.GetVolInfo(new FDSP_MsgHdrType(), new FDSP_GetVolInfoReqType(volumeName, 0));
        return toVolumeDescriptor(volumeDescriptor);
    }

    private VolumeDescriptor toVolumeDescriptor(FDSP_VolumeDescType volumeDescriptor) {
        return new VolumeDescriptor(volumeDescriptor.getVol_name(),
                                    DateTime.now().getMillis(),
                                    new VolumePolicy(2 * 1024 * 1024, VolumeConnector.S3));
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName) throws ApiException, TException {
        return client.ListVolumes(new FDSP_MsgHdrType())
                .stream()
                .map(this::toVolumeDescriptor)
                .collect(Collectors.toList());
    }
}
