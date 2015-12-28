/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import org.junit.Test;

import java.net.InetAddress;
import java.net.UnknownHostException;

import static com.google.common.net.InetAddresses.forString;
import static org.junit.Assert.assertTrue;
import static org.mockito.BDDMockito.given;
import static org.mockito.Mockito.mock;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

/**
 * @author ptinius
 */
public class NfsClientsTest
{
    @Test( expected = IllegalArgumentException.class )
    public void testInvalidSpecWithName( )
        throws UnknownHostException
    {
        NfsClients.forPattern( "www.formationds.com/24" );
    }

    @Test( expected = IllegalArgumentException.class )
    public void testInvalidSpecWithTooManySlashes( )
        throws UnknownHostException
    {
        NfsClients.forPattern( "1.1.1.1/24/25" );
    }

    @Test( expected = IllegalArgumentException.class )
    public void testInvalidSpecWithNegMask( )
        throws UnknownHostException
    {
        NfsClients.forPattern( "1.1.1.1/-7" );
    }

    @Test( expected = IllegalArgumentException.class )
    public void testInvalidSpecWithTooBigMaskV4( )
        throws UnknownHostException
    {
        NfsClients.forPattern( "1.1.1.1/33" );
    }

    @Test( expected = IllegalArgumentException.class )
    public void testInvalidSpecWithTooBigMaskV6( )
        throws UnknownHostException
    {
        NfsClients.forPattern( "fe80::3FF:F00D:BAD:F00D/129" );
    }

    @Test
    public void testForIpString( )
        throws UnknownHostException
    {
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "fe80::0:0:0:0/70" );
        assertEquals( NfsClients.IpAddressMatcher.class, ipMatcher.getClass( ) );
    }

    @Test
    public void testForRegexpBigString( )
        throws UnknownHostException
    {
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "a*.b.c" );
        assertEquals( NfsClients.RegexpNameMatcher.class, ipMatcher.getClass( ) );
    }

    @Test
    public void testForRegexpSmallString( )
        throws UnknownHostException
    {
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "a.b?.c" );
        assertEquals( NfsClients.RegexpNameMatcher.class, ipMatcher.getClass( ) );
    }

    @Test
    public void testForHostString( )
        throws UnknownHostException
    {
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "www.google.com" );
        assertEquals( NfsClients.HostNameMatcher.class, ipMatcher.getClass( ) );
    }

    @Test
    public void testIpV6SuccessfulIpNetMatching( )
        throws UnknownHostException
    {
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "fe80::0:0:0:0/70" );
        assertTrue( "Failed to match host with netmask.",
                    ipMatcher.apply( forString( "fe80::3FF:F00D:BAD:F00D" ) ) );
    }

    @Test
    public void testIpV4SuccessfulIpNetMatching( )
        throws UnknownHostException
    {
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "1.1.1.1/16" );


        assertTrue( "Failed to match host with netmask.",
                    ipMatcher.apply( forString( "1.1.2.3" ) ) );
    }

    @Test
    public void testDomainWildcart1( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "www.*.com" );
        assertTrue( "failed to match host by domain", ipMatcher.matcher( addr ) );
    }

    @Test
    public void testDomainWildcart2( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "www.f*.com" );
        assertTrue( "failed to match host by domain", ipMatcher.matcher( addr ) );
    }

    @Test
    public void testDomainWildcartNoMatch( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "www.e*.com" );
        assertFalse( "incorrect host matched by domain", ipMatcher.matcher( addr ) );
    }

    @Test
    public void testDomainOneChar( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www.formationds.com", "1.1.1.1" );
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "ww?.formationds.com" );
        assertTrue( "failed to match host by domain", ipMatcher.matcher( addr ) );
    }

    @Test
    public void testDomainOneCharNoMatch( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "www1.formationds.com", "1.1.1.1" );
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "ww?.formationds.com" );
        assertFalse( "incorrect host matched by domain", ipMatcher.matcher( addr ) );
    }

    @Test
    public void testDomainNoMatch( )
        throws UnknownHostException
    {
        InetAddress addr = mockInetAddress( "ww1.formationds.com", "1.1.1.1" );
        NfsClients.NfsClientMatcher ipMatcher = NfsClients.forPattern( "www.formationds.com" );
        assertFalse( "incorrect host matched by domain", ipMatcher.matcher( addr ) );
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
