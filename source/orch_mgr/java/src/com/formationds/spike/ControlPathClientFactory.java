package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ControlPathReq;
import FDS_ProtocolInterface.FDSP_RegisterNodeType;
import com.google.common.net.InetAddresses;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TProtocol;

import java.net.Inet4Address;

public class ControlPathClientFactory {
    public FDSP_ControlPathReq.Iface obtainClient(FDSP_RegisterNodeType node) {
        try {
            Inet4Address inet4Address = InetAddresses.fromInteger((int) node.getIp_lo_addr());
            String ipAsString = inet4Address.getHostAddress();
            TProtocol protocol = new FdspClient().handshake(ipAsString, node.getControl_port());
            FDSP_ControlPathReq.Client client = new FDSP_ControlPathReq.Client(protocol);
            return client;
        } catch (TException e) {
            throw new RuntimeException(e);
        }
    }
}
