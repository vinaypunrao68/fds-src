/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.common.base.Predicate;
import com.google.common.net.InetAddresses;
import com.google.common.net.InternetDomainName;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.regex.Pattern;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.net.InetAddresses.forString;
import static com.google.common.net.InetAddresses.isInetAddress;
import static com.google.common.primitives.Ints.fromByteArray;
import static com.google.common.primitives.Longs.fromBytes;

/**
 * @author ptinius
 */
public class NfsClients
{
    public class Builder
    {
        private String client = "*";

        public Builder forClient( final String client )
        {
            checkArgument( isValidHostSpecifier( client ), "bad host specifier: " + client);
            this.client = client;
            return this;
        }

        public NfsClients build( )
            throws UnknownHostException
        {
            return new NfsClients( this );
        }

        private boolean isValidHostSpecifier( String s )
        {
            int maskIdx = s.indexOf( '/' );

            String host;
            String mask;
            if ( maskIdx < 0 )
            {
                host = s;
                mask = "128";
            }
            else
            {
                host = s.substring( 0, maskIdx );
                mask = s.substring( maskIdx + 1 );
            }

            return ( isValidIpAddress( host ) && isValidNetmask( mask ) ) ||
                   ( isValidHostName( host ) || isValidWildcard( host ) );
        }

        private boolean isValidIpAddress( String s )
        {
            try
            {
                InetAddresses.forString( s );
                return true;
            }
            catch ( IllegalArgumentException ignore )
            {
            }

            return false;
        }

        private boolean isValidHostName( String s )
        {
            return InternetDomainName.isValid( s );
        }

        private boolean isValidWildcard( String s )
        {
            return isValidHostName( s.replace( '?', 'a' )
                                     .replace( '*', 'a' ) );
        }

        private boolean isValidNetmask( String s )
        {
            try
            {
                int mask = Integer.parseInt( s );
                return mask >= 0 && mask <= 128;
            }
            catch ( NumberFormatException ignore )
            {
            }

            return false;
        }
    }

    public static NfsClientMatcher forPattern( String s )
        throws UnknownHostException
    {
        String hostAndMask[] = s.split( "/" );
        checkArgument( hostAndMask.length < 3, "Invalid host specification: " + s );

        if ( !isInetAddress( hostAndMask[ 0 ] ) )
        {
            checkArgument( hostAndMask.length == 1,
                           "Invalid host specification (hostname with mask): " + s );
            if ( s.indexOf( '*' ) != -1 || s.indexOf( '?' ) != -1 )
            {
                return new RegexpNameMatcher( toRegExp( s ) );
            }
            else
            {
                return new HostNameMatcher( s );
            }
        }

        InetAddress net = forString( hostAndMask[ 0 ] );
        if ( hostAndMask.length == 2 )
        {
            return new IpAddressMatcher( s, net, Integer.parseInt( hostAndMask[ 1 ] ) );
        }

        return new IpAddressMatcher( s, net );
    }

    abstract static class NfsClientMatcher
        implements Predicate<InetAddress>
    {
        private final String pattern;

        protected NfsClientMatcher( final String pattern )
        {
            this.pattern = pattern;
        }

        public boolean matcher( final InetAddress inetAddress )
        {
            return apply( inetAddress );
        }

        public String getPattern( )
        {
            return pattern;
        }
    }

    static class IpAddressMatcher
        extends NfsClientMatcher
    {
        private static final int IPv4_FULL_MASK = 32;
        private static final int IPv6_FULL_MASK = 128;
        private static final int IPv6_HALF_MASK = 64;

        private final byte[] netBytes;
        private final int mask;

        private static int fullMaskOf( InetAddress address )
        {
            if ( address instanceof Inet4Address )
            {
                return IPv4_FULL_MASK;
            }

            if ( address instanceof Inet6Address )
            {
                return IPv6_FULL_MASK;
            }

            throw new IllegalArgumentException( "Unsupported Inet type: " + address.getClass( )
                                                                                   .getName( ) );
        }

        public IpAddressMatcher( String pattern, InetAddress subnet )
        {
            this( pattern, subnet, fullMaskOf( subnet ) );
        }

        public IpAddressMatcher( String pattern, InetAddress subnet, int mask )
        {
            super( pattern );
            this.netBytes = subnet.getAddress( );
            this.mask = mask;
            checkArgument( mask >= 0, "Netmask should be positive" );


            if ( this.netBytes.length == 4 )
            {
                checkArgument( mask <= IPv4_FULL_MASK,
                               "Netmask for ipv4 out-of-range upper range " + IPv4_FULL_MASK );
            }
            else
            {
                checkArgument( mask <= IPv6_FULL_MASK,
                               "Netmask for ipv6 out-of-range, upper range " + IPv6_FULL_MASK );
            }
        }

        @Override
        public boolean apply( InetAddress ip )
        {
            byte[] ipBytes = ip.getAddress( );
            if ( ipBytes.length != netBytes.length )
            {
                return false;
            }

            if ( ipBytes.length == 4 )
            {
                /*
                 * IPv4 can be represented as a 32 bit ints.
                 */
                int ipAsInt = fromByteArray( ipBytes );
                int netAsBytes = fromByteArray( netBytes );

                return ( ipAsInt ^ netAsBytes ) >> ( IPv4_FULL_MASK - mask ) == 0;
            }

            /**
             * IPv6 can be represented as two 64 bit longs.
             *
             * We evaluate second long only if bitmask bigger than 64. The
             * second longs are created only if needed as it turned to be the
             * slowest part.
             */
            long ipAsLong0 = fromBytes( ipBytes[ 0 ], ipBytes[ 1 ], ipBytes[ 2 ], ipBytes[ 3 ],
                                        ipBytes[ 4 ], ipBytes[ 5 ], ipBytes[ 6 ], ipBytes[ 7 ] );
            long netAsLong0 = fromBytes( netBytes[ 0 ], netBytes[ 1 ], netBytes[ 2 ], netBytes[ 3 ],
                                         netBytes[ 4 ], netBytes[ 5 ], netBytes[ 6 ],
                                         netBytes[ 7 ] );
            if ( mask > 64 )
            {
                long ipAsLong1 = fromBytes( ipBytes[ 8 ], ipBytes[ 9 ], ipBytes[ 10 ],
                                            ipBytes[ 11 ],
                                            ipBytes[ 12 ], ipBytes[ 13 ], ipBytes[ 14 ],
                                            ipBytes[ 15 ] );

                long netAsLong1 = fromBytes( netBytes[ 8 ], netBytes[ 9 ], netBytes[ 10 ],
                                             netBytes[ 11 ],
                                             netBytes[ 12 ], netBytes[ 13 ], netBytes[ 14 ],
                                             netBytes[ 15 ] );

                return ( ipAsLong0 == netAsLong0 )
                       & ( ipAsLong1 ^ netAsLong1 ) >> ( IPv6_FULL_MASK - mask ) == 0;
            }

            return ( ipAsLong0 ^ netAsLong0 ) >> ( IPv6_HALF_MASK - mask ) == 0;
        }
    }

    private static String toRegExp(String s) {
        return s.replace(".", "\\.").replace("?", ".").replace("*", ".*");
    }

    static class RegexpNameMatcher
        extends NfsClientMatcher
    {
        private final Pattern regexpPattern;

        public RegexpNameMatcher( String pattern )
        {
            super( pattern );
            this.regexpPattern = Pattern.compile( pattern );
        }

        @Override
        public boolean apply( InetAddress ip )
        {
            return regexpPattern.matcher( ip.getHostName( ) )
                                .matches( );
        }
    }

    static class HostNameMatcher
        extends NfsClientMatcher
    {
        HostNameMatcher( String hostname )
            throws UnknownHostException
        {
            super( hostname );
        }

        @Override
        public boolean apply( final InetAddress ip )
        {
            try
            {
                InetAddress[] addrs = InetAddress.getAllByName( getPattern( ) );
                for ( InetAddress addr : addrs )
                {
                    if ( addr.equals( ip ) )
                    {
                        return true;
                    }
                }
            }
            catch ( UnknownHostException e )
            {
                return false;
            }


            return false;
        }
    }

    private final NfsClientMatcher matcher;

    NfsClients( final Builder builder )
        throws UnknownHostException
    {
        this.matcher = forPattern( builder.client );
    }

    public String getPattern( ) { return this.matcher.getPattern(); }
}
