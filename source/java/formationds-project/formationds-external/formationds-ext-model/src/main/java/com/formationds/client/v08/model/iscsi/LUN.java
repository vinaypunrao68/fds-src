/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import java.util.Objects;

/**
 * @author ptinius
 */
public class LUN
{
    // IQN ( iSCSI qualified name ) target
    private static final String IQN = "iqn";
    // yyyy-mm is the year and month when the naming authority was established.
    private static final String YYYY_MM = "2012-05";
    // is usually reverse syntax of the Internet domain name of the naming authority.
    private static final String NAMING_AUTHORITY = "com.formationds.com";
    // the format of the iscsi qualified name
    private static final String IQN_FMT = IQN + "." + YYYY_MM + "." + NAMING_AUTHORITY + ":%s";

    public enum AccessType { RW, RO }

    public static class Builder {
        private AccessType accessType = AccessType.RW;
        private String lunName;

        protected AccessType getAccessType( ) { return accessType; }
        protected String getLunName( ) { return lunName; }

        public Builder withAccessType( final AccessType accessType )
        {
            this.accessType = accessType;
            return this;
        }

        public Builder withLun( final String lunName )
        {
            this.lunName = lunName;
            return this;
        }

        public LUN build() {
            return new LUN( getLunName(), getAccessType() );
        }
    }

    private final AccessType accessType;
    private final String lunName;
    private final String iqn;

    /**
     * @param lunName the name of the logical unit number
     * @param accessType the access control
     */
    protected LUN( final String lunName,
                   final AccessType accessType )
    {
        this.lunName = lunName;
        this.accessType = accessType;
        this.iqn = String.format( IQN_FMT, this.lunName );
     }

    /**
     * @return Returns the access type of this Logical Unit Number
     */
    public AccessType getAccessType( ) { return accessType; }

    /**
     * @return Returns the name of this Logical Unit Number
     */
    public String getLunName( ) { return lunName; }

    /**
     * @return Returns the iSCSI qualified name, i.e. IQN
     */
    public String getIQN( ) { return iqn; }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof LUN ) ) return false;
        final LUN lun = ( LUN ) o;
        return getAccessType( ) == lun.getAccessType( ) &&
            Objects.equals( getLunName( ), lun.getLunName( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( getAccessType( ), getLunName( ) );
    }

    @Override
    public String toString( )
    {
        return getLunName() + "(" + getAccessType() + ") ";
    }
}
