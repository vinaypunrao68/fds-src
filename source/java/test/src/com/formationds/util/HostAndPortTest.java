package com.formationds.util;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class HostAndPortTest {

    @Test
    public void testIpV6() {
        String s = "[2001:db8:85a3:8d3:1319:8a2e:370:7348]:8888";
        HostAndPort hostAndPort = HostAndPort.parseWithDefaultPort(s, 8888);
        assertEquals("[2001:db8:85a3:8d3:1319:8a2e:370:7348]", hostAndPort.getHost());
        assertEquals(8888, hostAndPort.getPort());
    }

    @Test
    public void testIpV6DefaultPort() {
        String s = "[2001:db8:85a3:8d3:1319:8a2e:370:7348]";
        HostAndPort hostAndPort = HostAndPort.parseWithDefaultPort(s, 8888);
        assertEquals("[2001:db8:85a3:8d3:1319:8a2e:370:7348]", hostAndPort.getHost());
        assertEquals(8888, hostAndPort.getPort());
        assertEquals("[2001:db8:85a3:8d3:1319:8a2e:370:7348]:8888", hostAndPort.toString());
    }

    @Test
    public void testHostname() {
        HostAndPort hostAndPort = HostAndPort.parseWithDefaultPort("foo:42", 43);
        assertEquals("foo", hostAndPort.getHost());
        assertEquals(42, hostAndPort.getPort());
    }

    @Test
    public void testDefaultPort() {
        HostAndPort hostAndPort = HostAndPort.parseWithDefaultPort("foo", 43);
        assertEquals("foo", hostAndPort.getHost());
        assertEquals(43, hostAndPort.getPort());
    }
}