package com.formationds.fdsp;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.google.common.net.InetAddresses;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;

import java.net.Inet4Address;

public class LegacyClientFactory {
    public FDSP_ControlPathReq.Iface controlPathClient(FDSP_RegisterNodeType node) throws TException {
        return new FDSP_ControlPathReq.Client(hanshake(node, node.getControl_port()));
    }

    public FDSP_ControlPathReq.Iface controlPathClient(String host, int port) throws TException {
        return new FDSP_ControlPathReq.Client(handshake(host, port));
    }

    public FDSP_DataPathReq.Iface dataPathClient(FDSP_RegisterNodeType node) throws TException {
        return new FDSP_DataPathReq.Client(hanshake(node, node.getData_port()));
    }

    public FDSP_DataPathReq.Iface dataPathClient(String host, int port) throws TException {
        return new FDSP_DataPathReq.Client(handshake(host, port));
    }

    public FDSP_MetaDataPathReq.Iface metadataPathClient(FDSP_RegisterNodeType node) throws TException {
        return new FDSP_MetaDataPathReq.Client(hanshake(node, node.getData_port()));
    }

    public FDSP_MetaDataPathReq.Iface metadataPathClient(String host, int port) throws TException {
        return new FDSP_MetaDataPathReq.Client(handshake(host, port));
    }

    public FDSP_OMControlPathReq.Iface omControlpathClient(FDSP_RegisterNodeType node) throws TException {
        return new FDSP_OMControlPathReq.Client(hanshake(node, node.getControl_port()));
    }

    public FDSP_OMControlPathReq.Iface omControlPathClient(String host, int port) throws TException {
        return new FDSP_OMControlPathReq.Client(handshake(host, port));
    }

    public FDSP_ConfigPathReq.Iface configPathClient(FDSP_RegisterNodeType node) throws TException {
        return new FDSP_ConfigPathReq.Client(hanshake(node, node.getControl_port()));
    }

    public FDSP_ConfigPathReq.Iface configPathClient(String host, int port) {
        return new FDSP_ConfigPathReq.Client(handshake(host, port));
    }

    private TProtocol hanshake(FDSP_RegisterNodeType node, int port) {
        Inet4Address inet4Address = InetAddresses.fromInteger((int) node.getIp_lo_addr());
        return handshake(inet4Address.getHostAddress(), port);
    }

    private TProtocol handshake(String host, int port)  {
        try {
            TSocket socket = new TSocket(host, port);
            socket.open();
            TProtocol protocol = new TBinaryProtocol(new TFramedTransport(socket));
            FDSP_Service.Client client = new FDSP_Service.Client(protocol);
            FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
            FDSP_SessionReqResp response = client.EstablishSession(msg);
            return protocol;
        } catch (TException e) {
            throw new RuntimeException(e);
        }
    }
}
