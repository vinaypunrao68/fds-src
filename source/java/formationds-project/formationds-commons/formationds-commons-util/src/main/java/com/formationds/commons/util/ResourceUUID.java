/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.protocol.svc.types.FDSP_MgrIdType;
import com.formationds.protocol.svc.types.SvcID;
import com.formationds.protocol.svc.types.SvcUuid;

import java.math.BigInteger;
import java.util.Objects;

/**
 * @author ptinius
 */
class ResourceUUID
{
    private static final int UUID_TYPE_SHIFT = 6;
    private static final BigInteger HEX_ONE =
        BigInteger.valueOf( 0x1L );
    private static final BigInteger UUID_MASK =
        BigInteger.valueOf( HEX_ONE.shiftLeft( UUID_TYPE_SHIFT ).longValue()
                            - 1L );

    private final BigInteger uuid;

    /**
     * @param hexUuid a {@link String} representing the hexadecimal {@link SvcID}
     */
    public ResourceUUID( final String hexUuid )
    {
        this( new BigInteger( hexUuid, 16 ) );
    }

    /**
     * @param uuid a {@link BigInteger} representing the {@link SvcID}
     */
    public ResourceUUID( final BigInteger uuid )
    {
        this.uuid = uuid;
    }

    /**
     * @return Returns the {@link FDSP_MgrIdType}
     */
    public FDSP_MgrIdType type()
    {
        return FDSP_MgrIdType.findByValue( uuid.and( UUID_MASK )
                                               .intValue() );
    }

    /**
     * @return Returns {@link SvcUuid}
     */
    public SvcUuid uuid()
    {
        return new SvcUuid( uuid.longValue() );
    }

    /**
     * @return Returns this converted to a {@code long}.
     */
    public long longValue()
    {
        return uuid.longValue();
    }

    /**
     * @return a hash code value for this object.
     */
    @Override
    public int hashCode( )
    {
        return Objects.hashCode( this );
    }

    /**
     * @param o the reference object with which to compare.
     *
     * @return {@code true} if this object is the same as the obj argument;
     *         {@code false} otherwise.
     */
    @Override
    public boolean equals( final Object o )
    {
        return ( this == o ) ||
               ( ( o instanceof ResourceUUID ) &&
                 Objects.equals( uuid, o ) );
    }

    /**
     * @return Returns {@link String} representing this object
     */
    @Override
    public String toString( )
    {
        return String.valueOf( String.valueOf( uuid.longValue( ) ) );
    }

    /**
     * @param uuid the {@link ResourceUUID} representing the service uuid
     * @param type the {@link FDSP_MgrIdType} representing the type of service to get uuid for.
     *
     * @return Returns
     */
    public static SvcUuid getUuid4Type( final SvcUuid uuid,
                                        final FDSP_MgrIdType type )
    {
        return new SvcUuid( BigInteger.valueOf( uuid.getSvc_uuid() )
                                                    .andNot( UUID_MASK )
                                      .or( BigInteger.valueOf( type.getValue( ) ) )
                                      .longValue() );
    }
}
