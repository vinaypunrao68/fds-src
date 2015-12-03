package com.formationds.sc;
import com.formationds.apis.*;
import com.formationds.protocol.dm.QueryCatalogMsg;
import com.formationds.protocol.om.OMSvc;
import com.formationds.protocol.sm.GetObjectMsg;
import com.formationds.protocol.sm.GetObjectResp;
import com.formationds.protocol.sm.PutObjectMsg;
import com.formationds.protocol.sm.PutObjectRspMsg;
import com.formationds.protocol.svc.CtrlNotifyDLTUpdate;
import com.formationds.protocol.svc.CtrlNotifyDMTUpdate;
import com.formationds.protocol.svc.types.*;
import com.formationds.util.blob.Mode;
import com.formationds.xdi.RealAsyncAm;
import com.google.common.net.HostAndPort;
import org.apache.commons.codec.digest.DigestUtils;
import org.junit.Assert;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

// this is a scratch pad set of tests
// don't worry about deleting them if it comes to that
public class StorageClientTest {
    static final int DEFAULT_OM_PORT = 7004;
    static final int DEFAULT_CS_PORT = 9090;
    static final int DEFAULT_AMSVC_PORT = 8899;
    static final String DOMAIN_PLACEHOLDER = "FDS";

    int recvPort = 10294;
    long uuid = 102207;

    //@Test
    public void serializeDeserializeDlt() throws Exception {
        HostAndPort omAddress = HostAndPort.fromParts("127.0.0.1", DEFAULT_OM_PORT);
        HostAndPort recvAddress = HostAndPort.fromParts("127.0.0.1", recvPort);
        try(SvcState io = new SvcState(recvAddress, omAddress, uuid)) {
            io.openAndRegister();

            OMSvc.Iface omClient = io.getClientFactory().omClient(omAddress);

            CtrlNotifyDMTUpdate dmtData = omClient.getDMT(0);
            CtrlNotifyDLTUpdate dltData = omClient.getDLT(0);

            ByteBuffer dltBuf = dltData.getDlt_data().bufferForDlt_data().duplicate();
            ByteBuffer dmtBuf = dmtData.getDmt_data().bufferForDmt_data().duplicate();
            Dlt dlt = new Dlt(dltBuf);
            Dmt dmt = new Dmt(dmtBuf);

            System.out.println("sup");
        }
    }

    //@Test
    public void basicTest() throws Exception {
        HostAndPort omAddress = HostAndPort.fromParts("127.0.0.1", DEFAULT_OM_PORT);
        HostAndPort recvAddress = HostAndPort.fromParts("127.0.0.1", recvPort);
        try(SvcState io = new SvcState(recvAddress, omAddress, uuid)) {
            String volumeName = UUID.randomUUID().toString();
            long volumeId = createAndPopulateVolume(io, volumeName);

            io.openAndRegister();
            OMSvc.Iface omClient = io.getClientFactory().omClient(omAddress);
            SvcInfo myInfo = omClient.getSvcInfo(new SvcUuid(uuid));

            Assert.assertTrue(myInfo.getSvc_status() == ServiceStatus.SVC_STATUS_ACTIVE);

            List<SvcInfo> svcMap = omClient.getSvcMap(0);
            Optional<SvcInfo> dmInfoOpt = svcMap.stream().filter(si -> si.getSvc_type() == FDSP_MgrIdType.FDSP_DATA_MGR).findFirst();
            Optional<SvcInfo> smInfoOpt = svcMap.stream().filter(si -> si.getSvc_type() == FDSP_MgrIdType.FDSP_STOR_MGR).findFirst();
            Assert.assertTrue(dmInfoOpt.isPresent());
            SvcInfo dmInfo = dmInfoOpt.get();

            PlatNetSvcChannel dmChannel = io.getChannel(dmInfo.getSvc_id().getSvc_uuid().getSvc_uuid());

            QueryCatalogMsg msg = new QueryCatalogMsg(volumeId, "blob0", 0, Long.MAX_VALUE, -1, null, null, -1);
            QueryCatalogMsg responseMessage = dmChannel.call(FDSPMsgTypeId.QueryCatalogMsgTypeId, msg, 30, TimeUnit.SECONDS)
                    .deserializeInto(FDSPMsgTypeId.QueryCatalogMsgTypeId, new QueryCatalogMsg())
                    .get();

            FDS_ObjectIdType firstObject = responseMessage.getObj_list().get(0).getData_obj_id();
            GetObjectMsg getObjectRequest = new GetObjectMsg(volumeId, firstObject);

            PlatNetSvcChannel smChannel = io.getChannel(smInfoOpt.get().getSvc_id().getSvc_uuid().getSvc_uuid());
            byte[] data = new byte[] { 2, 3, 4, 5 };
            PutObjectMsg putObjectMsg = new PutObjectMsg(volumeId, new FDS_ObjectIdType(ByteBuffer.wrap(DigestUtils.sha1(data))), false, data.length, ByteBuffer.wrap(data));
            PutObjectRspMsg putObjectRspMsg = smChannel.call(FDSPMsgTypeId.PutObjectMsgTypeId, putObjectMsg, 30, TimeUnit.SECONDS)
                    .deserializeInto(FDSPMsgTypeId.PutObjectRspMsgTypeId, new PutObjectRspMsg())
                    .get();

            GetObjectResp getResponseMessage = smChannel.call(FDSPMsgTypeId.GetObjectMsgTypeId, getObjectRequest, 30, TimeUnit.SECONDS)
                    .deserializeInto(FDSPMsgTypeId.GetObjectRespTypeId, new GetObjectResp())
                    .get();

            // Assert.assertEquals(4, getResponseMessage.data_obj.remaining());
            byte[] rd = new byte[4];
            getResponseMessage.data_obj.slice().get(rd);

            Assert.assertArrayEquals(new byte[] { 1, 2, 3, 4 }, rd);
            System.out.println("basic test complete");
        }
    }

    public long createAndPopulateVolume(SvcState svcIo, String volumeName) throws Exception {
        HostAndPort configServiceAddress = HostAndPort.fromParts("127.0.0.1", DEFAULT_CS_PORT);
        HostAndPort asyncAmAddress = HostAndPort.fromParts("127.0.0.1", DEFAULT_AMSVC_PORT);

        ConfigurationService.Iface configServiceClient = svcIo.getClientFactory().configServiceClient(configServiceAddress);

        long tenantId = configServiceClient.createTenant("skoopy");
        configServiceClient.createVolume(DOMAIN_PLACEHOLDER, volumeName, new VolumeSettings(2 * 1024 * 1024, VolumeType.OBJECT, -1, 0, MediaPolicy.HDD_ONLY), tenantId);

        RealAsyncAm am = new RealAsyncAm(asyncAmAddress.getHostText(), asyncAmAddress.getPort(), 12930, org.joda.time.Duration.millis(30000));
        am.start();
        byte[] content = new byte[] { 1, 2, 3, 4 };
        am.updateBlobOnce(DOMAIN_PLACEHOLDER, volumeName, "blob0", Mode.TRUNCATE.getValue(), ByteBuffer.wrap(content), content.length, new ObjectOffset(0), Collections.emptyMap()).join();
        ByteBuffer blob0 = am.getBlob(DOMAIN_PLACEHOLDER, volumeName, "blob0", content.length, new ObjectOffset(0)).get();

        return configServiceClient.getVolumeId(volumeName);
    }
}