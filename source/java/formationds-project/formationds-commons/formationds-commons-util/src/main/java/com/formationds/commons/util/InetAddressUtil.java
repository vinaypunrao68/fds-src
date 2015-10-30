/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.util.regex.Pattern;

/**
 * @author ptinius
 */
public class InetAddressUtil
{
    private InetAddressUtil( )
    { /* singleton */ }

    private static final Pattern IPV4 =
            Pattern.compile( "^(25[0-5]|2[0-4]\\\\d|[0-1]?\\\\d?\\\\d)(\\\\.(25[0-5]|2[0-4]\\\\d|[0-1]?\\\\d?\\\\d)){3}$" );
    private static final Pattern IPV6 =
            Pattern.compile( "^(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$" );
    private static final Pattern IPV6_HEX_COMPRESSED =
            Pattern.compile( "^((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?)::((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?)$" );

    /**
     * @param input the {@link String} representing the dot-decimal notation, which consists of
     *              four octets of the address expressed individually in decimal numbers and
     *              separated by periods. In addition, IPv6 addresses often contain consecutive
     *              hexadecimal fields of zeros. To simplify address entry, two colons ( :: ) can
     *              be used to represent the consecutive fields of zeros when typing the
     *              IPv6 address.
     *
     * @return Returns {@code true} if a valid formatted IPv6 address
     */
    public static boolean isIPv4Address( final String input )
    {
        return IPV4.matcher( input )
                   .matches( );
    }

    /**
     * @param input the {@link String} representing as eight groups of four hexadecimal
     *              ( 16-bits each ) digits with the groups being separated by colons.
     *
     * @return Returns {@code true} if a valid formatted IPv4 dot-decimal notation.
     */
    public static boolean isIPv6Address( final String input )
    {
        return isIPv6StdAddress( input ) || isIPv6HexCompressedAddress( input );
    }

    private static boolean isIPv6StdAddress( final String input )
    {
        return IPV6.matcher( input )
                   .matches( );
    }

    private static boolean isIPv6HexCompressedAddress( final String input )
    {
        return IPV6_HEX_COMPRESSED.matcher( input )
                                  .matches( );
    }
}
