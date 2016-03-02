package com.formationds.xdi.experimental;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.nfs.Chunker;
import com.formationds.nfs.IoOps;
import com.formationds.protocol.Credentials;
import com.formationds.protocol.Initiator;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.XdiConfigurationApi;
import com.formationds.xdi.contracts.ConnectorConfig;
import com.formationds.xdi.contracts.ConnectorData;
import com.formationds.xdi.contracts.thrift.config.*;
import com.formationds.xdi.contracts.thrift.data.DirectWriteRequest;
import com.formationds.xdi.contracts.thrift.data.ReadRequest;
import com.formationds.xdi.contracts.thrift.data.ReadResponse;
import com.formationds.xdi.contracts.thrift.data.SuccessResponse;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Collectors;

public class XdiConnector implements ConnectorData, ConnectorConfig {
    private XdiConfigurationApi configApi;
    private final Chunker chunker;

    public XdiConnector(XdiConfigurationApi configApi, IoOps ops) {
        this.configApi = configApi;
        chunker = new Chunker(ops);
    }

    @Override
    public CompletableFuture<ReadResponse> read(ReadRequest readRequest) {
        try {
            String volumeName = configApi.getVolumeName(readRequest.getVolumeId());
            int objectSize = configApi.statVolume("", volumeName).getPolicy().getMaxObjectSizeInBytes();
            byte[] dest = new byte[readRequest.getReadLength()];
            chunker.read("", volumeName, readRequest.getObjectName(), objectSize, dest, readRequest.getReadOffset(), readRequest.getReadLength());
            ReadResponse readResponse = new ReadResponse(ByteBuffer.wrap(dest));
            return CompletableFuture.completedFuture(readResponse);
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<SuccessResponse> writeDirect(DirectWriteRequest writeRequest) {
        try {
            String volumeName = configApi.getVolumeName(writeRequest.getVolumeId());
            int objectSize = configApi.statVolume("", volumeName).getPolicy().getMaxObjectSizeInBytes();
            chunker.write("", volumeName, writeRequest.getObjectName(), objectSize, writeRequest.getData(), writeRequest.getWriteOffset(), writeRequest.getData().length, x -> null);
            return CompletableFuture.completedFuture(new SuccessResponse());
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<VolumeListResponse> listAllVolumes() {
        List<VolumeDescriptor> vds = null;
        try {
            vds = configApi.listVolumes("");
        } catch (TException e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }

        List<Volume> volumes = new ArrayList<>();

        for (VolumeDescriptor vd : vds) {
            if (vd.getPolicy().getVolumeType().equals(com.formationds.apis.VolumeType.ISCSI)) {
                IscsiVolume iscsiVolume = new IscsiVolume(
                        vd.getPolicy().getBlockDeviceSizeInBytes(),
                        vd.getPolicy().getMaxObjectSizeInBytes(),
                        makeCredentials(vd.getPolicy().getIscsiTarget().getIncomingUsers()),
                        makeInitiators(vd.getPolicy().getIscsiTarget().getInitiators())
                );
                volumes.add(new Volume(vd.getName(), vd.getVolId(), VolumeType.iscsiVolume(iscsiVolume)));
            } else {
                return CompletableFutureUtility.exceptionFuture(new UnsupportedOperationException("Unsupported volume type"));
            }
        }
        VolumeListResponse response = new VolumeListResponse(volumes);
        return CompletableFuture.completedFuture(response);
    }

    private List<String> makeInitiators(List<Initiator> initiators) {
        return initiators
                .stream()
                .map(i -> i.getWwn_mask())
                .collect(Collectors.toList());
    }

    private List<IscsiCredentials> makeCredentials(List<Credentials> incomingUsers) {
        return incomingUsers
                .stream()
                .map(c -> new IscsiCredentials(c.getName(), c.getPasswd()))
                .collect(Collectors.toList());
    }
}
