package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import java.net.InetAddress;

public class MyIpTest {
    @Test
    public void testBestLocalAddress() throws Exception {
        InetAddress inetAddress = new MyIp().bestLocalAddress();
        System.out.println(inetAddress.getHostAddress());
    }
}
