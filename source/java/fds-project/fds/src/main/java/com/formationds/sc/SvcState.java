package com.formationds.sc;

import FDS_ProtocolInterface.FDSP_ActivateNodeType;
import com.formationds.protocol.*;
import com.formationds.protocol.om.OMSvc;
import com.formationds.protocol.svc.*;
import com.formationds.protocol.svc.types.*;
import com.google.common.net.HostAndPort;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

public class SvcState implements AutoCloseable, PlatNetSvc.Iface {
    private int incarnationNo = 0;
    private HostAndPort listenerAddress;

    private HostAndPort omAddress;
    private long uuid;

    private AwaitableResponseHandler handler;
    private PlatNetSvcHost host;
    private volatile Map<String, FDSP_VolumeDescType> volumeMap;
    private Object volumeMapLock = new Object();
    private long defaultChannelTimeout;
    private TimeUnit defaultChannelTimeoutUnit;

    public AwaitableResponseHandler getHandler() {
        return handler;
    }

    public ClientFactory getClientFactory() {
        return clientFactory;
    }

    private ClientFactory clientFactory;
    private AtomicLong currentMsgId;

    private Map<Long, SvcInfo> svcInfoMap;

    private volatile boolean isOpen;
    private Dlt dlt;
    private Dmt dmt;

    public SvcState(HostAndPort listenerAddress, HostAndPort omAddress, long uuid) {
        this.listenerAddress = listenerAddress;
        this.omAddress = omAddress;
        this.uuid = uuid;

        handler = new AwaitableResponseHandler();
        host = new PlatNetSvcHost(this, listenerAddress.getPort());
        clientFactory = new ClientFactory();
        currentMsgId = new AtomicLong(0);

        svcInfoMap = new HashMap<>();
        isOpen = false;
        volumeMap = new HashMap<>();

        defaultChannelTimeout = 30;
        defaultChannelTimeoutUnit = TimeUnit.SECONDS;
    }

    private SvcInfo getSvcInfo() {
        SvcInfo myInfo = new SvcInfo();
        myInfo.setSvc_id(new SvcID(new SvcUuid(uuid), "java-sc"));
        myInfo.setSvc_port(listenerAddress.getPort());
        myInfo.setSvc_status(ServiceStatus.SVC_STATUS_ACTIVE);
        myInfo.setIp(listenerAddress.getHostText());
        myInfo.setSvc_type(FDSP_MgrIdType.FDSP_ACCESS_MGR);
        myInfo.setName("java-sc");
        myInfo.setSvc_auto_name("");
        myInfo.setProps(Collections.emptyMap());
        myInfo.setIncarnationNo(incarnationNo);
        return myInfo;
    }

    public SvcState setDefaultChannelTimeout(long defaultChannelTimeout, TimeUnit defaultChannelTimeoutUnit) {
        this.defaultChannelTimeout = defaultChannelTimeout;
        this.defaultChannelTimeoutUnit = defaultChannelTimeoutUnit;
        return this;
    }

    public synchronized void openAndRegister() throws TException {
        if(isOpen)
            return;

        host.open();

        // register
        OMSvc.Iface omClient = omClient();
        SvcInfo svcInfo = pollOmSvcInfo(omClient);
        dlt = new Dlt(omClient.getDLT(0).dlt_data.bufferForDlt_data());
        dmt = new Dmt(omClient.getDMT(0).dmt_data.bufferForDmt_data());

        while(true) {
            if(svcInfo == null) {
                svcInfo = getSvcInfo();
            } else {
                svcInfo.setIncarnationNo(svcInfo.getIncarnationNo() + 1);
                svcInfo.setSvc_status(ServiceStatus.SVC_STATUS_ACTIVE);

            }

            omClient.registerService(svcInfo);
            svcInfo = pollOmSvcInfo(omClient);
            if(svcInfo.getSvc_status() != null && svcInfo.getSvc_status() == ServiceStatus.SVC_STATUS_ACTIVE)
                break;
        }

        incarnationNo = svcInfo.getIncarnationNo();
        isOpen = true;
    }

    public OMSvc.Iface omClient() {
        return clientFactory.omClient(omAddress);
    }

    public PlatNetSvcChannel getChannel(long dstUuid)  {
        SvcInfo svcInfo = null;
        synchronized (svcInfoMap) {
            svcInfo = svcInfoMap.get(dstUuid);
        }

        if(svcInfo != null) {
            HostAndPort hostAndPort = HostAndPort.fromParts(svcInfo.getIp(), svcInfo.getSvc_port());
            return new PlatNetSvcChannel(clientFactory.platNetSvcClient(hostAndPort), uuid, dstUuid, handler, currentMsgId::incrementAndGet)
                    .withDefaultTimeout(defaultChannelTimeout, defaultChannelTimeoutUnit);
        }

        // TODO: specific exception for this?
        return null;
    }

    public Dlt getDlt() {
        if(!isOpen)
            throw new IllegalStateException("must call openAndRegister() before getDlt()");

        return dlt;
    }

    public Dmt getDmt() {
        if(!isOpen)
            throw new IllegalStateException("must call openAndRegister() before getDmt()");

        return dmt;
    }

    private SvcInfo pollOmSvcInfo(OMSvc.Iface omClient) throws TException {
        SvcInfo svcInfo;List<SvcInfo> svcMap = omClient.getSvcMap(0);
        Optional<SvcInfo> info = svcMap.stream().filter(si -> si.getSvc_id().getSvc_uuid().getSvc_uuid() == uuid).findFirst();
        svcInfo = info.orElse(null);
        return svcInfo;
    }

    public Optional<FDSP_VolumeDescType> getVolumeDescriptor(String name) throws TException {
        FDSP_VolumeDescType descriptor = volumeMap.get(name);
        if(descriptor == null) {
            synchronized (volumeMapLock) {
                descriptor = volumeMap.get(name);
                if(descriptor == null) {
                    volumeMap = getVolumeDescriptors();
                    descriptor = volumeMap.get(name);

                    if(descriptor == null)
                        return Optional.empty();

                }
            }
        }

        return Optional.of(descriptor);
    }

    private Map<String, FDSP_VolumeDescType> getVolumeDescriptors() throws TException {
        GetAllVolumeDescriptors allVolumeDescriptors = omClient().getAllVolumeDescriptors(0L);
        Map<String, FDSP_VolumeDescType> descriptorMap = new HashMap<>();
        for(CtrlNotifyVolAdd vd : allVolumeDescriptors.getVolumeList())
            descriptorMap.put(vd.getVol_desc().getVol_name(), vd.getVol_desc());

        return descriptorMap;
    }

    @Override
    public synchronized void close() throws Exception {
        host.close();
        isOpen = false;
    }

    @Override
    public void asyncReqt(AsyncHdr asyncHdr, ByteBuffer payload) throws TException {
        switch(asyncHdr.getMsg_type_id()) {
            case UpdateSvcMapMsgTypeId: {
                synchronized (svcInfoMap) {
                    UpdateSvcMapMsg message = ThriftUtil.deserialize(payload, new UpdateSvcMapMsg());
                    for(SvcInfo info : message.getUpdates()) {
                        svcInfoMap.put(info.getSvc_id().getSvc_uuid().getSvc_uuid(), info);
                    }
                }
                break;
            }

            case CtrlNotifyDLTUpdateTypeId: {
                CtrlNotifyDLTUpdate dltUpdate = ThriftUtil.deserialize(payload, new CtrlNotifyDLTUpdate());
                dlt = new Dlt(dltUpdate.dlt_data.bufferForDlt_data());
                // TODO: wait for pending I/O to complete and respond with same message
                break;
            }
            case CtrlNotifyDMTUpdateTypeId: {
                CtrlNotifyDMTUpdate dmtUpdate = ThriftUtil.deserialize(payload, new CtrlNotifyDMTUpdate());
                dmt = new Dmt(dmtUpdate.dmt_data.bufferForDmt_data());
                // TODO: wait for pending I/O to complete and respond with same message
                break;
            }
            default:
                // do nothing
                break;
        }
    }

    @Override
    public void asyncResp(AsyncHdr asyncHdr, ByteBuffer payload) throws TException {
        handler.recieveResponse(asyncHdr, payload);
    }

    @Override
    public List<SvcInfo> getSvcMap(long nullarg) throws TException {
        return null;
    }

    @Override
    public void notifyNodeActive(FDSP_ActivateNodeType info) throws TException {

    }

    @Override
    public List<NodeInfoMsg> notifyNodeInfo(NodeInfoMsg info, boolean bcast) throws TException {
        return null;
    }

    @Override
    public DomainNodes getDomainNodes(DomainNodes dom) throws TException {
        return null;
    }

    @Override
    public ServiceStatus getStatus(int nullarg) throws TException {
        return ServiceStatus.SVC_STATUS_ACTIVE;
    }

    @Override
    public Map<String, Long> getCounters(String id) throws TException {
        return null;
    }

    @Override
    public void resetCounters(String id) throws TException {

    }

    @Override
    public void setConfigVal(String id, long value) throws TException {

    }

    @Override
    public void setFlag(String id, long value) throws TException {

    }

    @Override
    public long getFlag(String id) throws TException {
        return 0;
    }

    @Override
    public Map<String, Long> getFlags(int nullarg) throws TException {
        return null;
    }

    @Override
    public boolean setFault(String cmdline) throws TException {
        return false;
    }
}
