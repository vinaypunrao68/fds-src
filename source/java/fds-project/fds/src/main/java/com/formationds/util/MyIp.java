package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;

public class MyIp {
    public InetAddress bestLocalAddress() throws Exception {
        Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
        while (interfaces.hasMoreElements()){
            Enumeration<InetAddress> a = interfaces.nextElement().getInetAddresses();
            while (a.hasMoreElements()) {
                InetAddress inetAddress = a.nextElement();
                if (inetAddress instanceof Inet4Address) {
                    return inetAddress;
                }
            }
        }
        return InetAddress.getLocalHost();
    }
}
