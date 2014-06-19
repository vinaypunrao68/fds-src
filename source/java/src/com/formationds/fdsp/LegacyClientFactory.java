package com.formationds.fdsp;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Service;
import FDS_ProtocolInterface.FDSP_SessionReqResp;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;

public class LegacyClientFactory {
    public FDSP_ConfigPathReq.Iface configPathClient(String host, int port) {
        return new FDSP_ConfigPathReq.Client(handshake(host, port));
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
