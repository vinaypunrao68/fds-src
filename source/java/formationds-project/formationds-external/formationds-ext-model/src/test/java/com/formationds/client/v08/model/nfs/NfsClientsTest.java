/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import org.dcache.nfs.InetAddressMatcher;
import org.junit.Ignore;
import org.junit.Test;

import java.net.InetAddress;
import java.net.UnknownHostException;

import static com.google.common.net.InetAddresses.forString;
import static junit.framework.TestCase.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.BDDMockito.given;
import static org.mockito.Mockito.mock;

/**
 * @author ptinius
 */
public class NfsClientsTest
{
    @Test( expected = IllegalArgumentException.class )
    public void testInvalidNameWithMask( )
        throws UnknownHostException
    {
        new NfsClients( "www.formationds.com/24" );
    }

    @Test( expected = IllegalArgumentException.class )
    public void testInvalidNameWithTooManySlashes( )
        throws UnknownHostException
    {
        new NfsClients( "1.1.1.1/24/25" );
    }

    @Test( expected = IllegalArgumentException.class )
    public void testInvalidIpWithNegMask( )
        throws UnknownHostException
    {
        new NfsClients( "1.1.1.1/-7" );
    }


    @Test( expected = IllegalArgumentException.class )
    public void testInvalidIpWithTooBigMaskV4( )
        throws UnknownHostException
    {
        new NfsClients( "1.1.1.1/33" );
    }


    @Test( expected = IllegalArgumentException.class )
    public void testInvalidIpWithTooBigMaskV6( )
        throws UnknownHostException
    {
        new NfsClients( "fe80::3FF:F00D:BAD:F00D/129" );
    }

    @Test
    public void testForIpString( )
        throws UnknownHostException
    {
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "fe80::0:0:0:0/70" );
        assertEquals( InetAddressMatcher.IpAddressMatcher.class, ipMatcher.getClass( ) );
    }


    @Test
    public void testForRegexpBigString( )
        throws UnknownHostException
    {
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "a*.b.c" );
        assertEquals( InetAddressMatcher.RegexpNameMatcher.class, ipMatcher.getClass( ) );
    }


    @Test
    public void testForRegexpSmallString( )
        throws UnknownHostException
    {
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "a.b?.c" );
        assertEquals( InetAddressMatcher.RegexpNameMatcher.class, ipMatcher.getClass( ) );
    }


    @Test
    public void testForHostString( )
        throws UnknownHostException
    {
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "www.google.com" );
        assertEquals( InetAddressMatcher.HostNameMatcher.class, ipMatcher.getClass( ) );
    }


    @Test
    public void testIpV6SuccessfulIpNetMatching( )
        throws UnknownHostException
    {
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "fe80::0:0:0:0/70" );
        assertTrue( "Failed to match host with netmask.",
                    ipMatcher.apply( forString( "fe80::3FF:F00D:BAD:F00D" ) ) );
    }


    @Test
    public void testIpV4SuccessfulIpNetMatching( )
        throws UnknownHostException
    {
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "1.1.1.1/16" );
        assertTrue( "Failed to match host with netmask.",
                    ipMatcher.apply( forString( "1.1.2.3" ) ) );
    }


    @Test
    public void testDomainWildcart1( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "www.*.com" );
        assertTrue( "failed to match host by domain", ipMatcher.match( addr ) );
    }


    @Test
    public void testDomainWildcart2( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "www.f*.com" );
        assertTrue( "failed to match host by domain", ipMatcher.match( addr ) );
    }


    @Test
    public void testDomainWildcartNoMatch( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "www.b*.com" );
        assertFalse( "incorrect host matched by domain", ipMatcher.match( addr ) );
    }


    @Test
    public void testDomainOneChar( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "ww?.formationds.com" );
        assertTrue( "failed to match host by domain", ipMatcher.match( addr ) );
    }


    @Test
    public void testDomainOneCharNoMatch( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www1.formationds.com", "1.1.1.1" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "ww?.formationds.com" );
        assertFalse( "incorrect host matched by domain", ipMatcher.match( addr ) );
    }


    @Test
    @Ignore
    public void testDomainMatch( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "www.formationds.com" );
        assertTrue( "failed to match host by domain", ipMatcher.match( addr ) );
    }


    @Test
    @Ignore
    public void testDomainMatchMultipleIPs( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1",
                                            "fe80::3FF:F00D:BAD:F00D" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "www.formationds.com" );
        assertTrue( "failed to match host by domain", ipMatcher.match( addr ) );
    }


    @Test
    public void testDomainNoMatch( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "ww1.formationds.com", "1.1.1.1" );
        InetAddressMatcher ipMatcher = InetAddressMatcher.forPattern( "www.formationds.com" );
        assertFalse( "incorrect host matched by domain", ipMatcher.match( addr ) );
    }


    private InetAddress mockInetAddress( String dnsName, String... ips )
        throws UnknownHostException
    {
        InetAddress mockedAddress = mock( InetAddress.class );
        given( mockedAddress.getHostName( ) ).willReturn( dnsName );
        given( mockedAddress.getCanonicalHostName( ) ).willReturn( dnsName );

        return mockedAddress;
    }

}
