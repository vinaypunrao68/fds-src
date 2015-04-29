package com.formationds.util;

import org.junit.Test;

import java.net.InetSocketAddress;
import java.net.ServerSocket;

import static org.junit.Assert.*;

public class ServerPortFinderTest {
    @Test
    public void testFindServerPort() throws Exception {
        int port = 20000;
        new ServerSocket(port);
        int availablePort = new ServerPortFinder().findPort("panda", port);
        assertEquals(port + 1, availablePort);
        new ServerSocket(port + 1);
    }
}