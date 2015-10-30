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

    public static boolean isIPv4Address( final String input )
    {
        return IPV4.matcher( input )
                   .matches( );
    }

    public static boolean isIPv6StdAddress( final String input )
    {
        return IPV6.matcher( input )
                   .matches( );
    }

    public static boolean isIPv6HexCompressedAddress( final String input )
    {
        return IPV6_HEX_COMPRESSED.matcher( input )
                                  .matches( );
    }

    public static boolean isIPv6Address( final String input )
    {
        return isIPv6StdAddress( input ) || isIPv6HexCompressedAddress( input );
    }
}
