package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.fdsp.ClientFactory;
import com.formationds.util.MyIp;
import com.google.common.net.InetAddresses;

import java.net.InetAddress;
import java.util.UUID;

public class RegisterNode {
    ClientFactory clientFactory;

    public RegisterNode(ClientFactory clientFactory) {
        this.clientFactory = clientFactory;
    }

    public void execute(FDSP_MgrIdType nodeType, UUID uuid, String omHost, int myPort, int omControlPort) throws Exception {
        FDSP_OMControlPathReq.Iface client = clientFactory.omControlPathClient(omHost, omControlPort);
        FDSP_RegisterNodeType nodeMesg = new FDSP_RegisterNodeType();
        InetAddress inetAddress = new MyIp().bestLocalAddress();
        nodeMesg.setNode_type(nodeType);
        nodeMesg.setNode_name(inetAddress.getHostAddress());
        nodeMesg.setService_uuid(new FDSP_Uuid(uuid.getMostSignificantBits()));
        int ipInt = InetAddresses.coerceToInteger(inetAddress);
        nodeMesg.setIp_lo_addr(ipInt);
        nodeMesg.setControl_port(myPort);
        nodeMesg.setData_port(myPort);
        client.RegisterNode(new FDSP_MsgHdrType(), nodeMesg);
    }
}
