package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Service;
import FDS_ProtocolInterface.FDSP_SessionReqResp;
import org.apache.thrift.TException;
import org.apache.thrift.TServiceClient;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TSocket;

public class FdspClient<T extends TServiceClient> {
    public TProtocol handshake(String host, int port) throws TException {
        TSocket socket = new TSocket(host, port);
        socket.open();
        TProtocol protocol = new TBinaryProtocol(socket);
        FDSP_Service.Client client = new FDSP_Service.Client(protocol);
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        FDSP_SessionReqResp response = client.EstablishSession(msg);
        return protocol;
    }
}
