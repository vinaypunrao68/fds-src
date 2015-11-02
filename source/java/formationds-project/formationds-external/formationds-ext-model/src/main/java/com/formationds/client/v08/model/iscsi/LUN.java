/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

/**
 * @author ptinius
 */
public class LUN
{
    public enum AccessType { RW, RO }

    public static class Builder {
        private AccessType accessType;
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

    /**
     * @param lunName the name of the logical unit number
     * @param accessType the access control
     */
    protected LUN( final String lunName,
                   final AccessType accessType )
    {
        this.lunName = lunName;
        this.accessType = accessType;
    }

    /**
     * @return Returns the access type of this Logical Unit Number
     */
    public AccessType getAccessType( )
    {
        return accessType;
    }

    /**
     * @return Returns the name of this Logical Unit Number
     */
    public String getLunName( )
    {
        return lunName;
    }

    @Override
    public String toString( )
    {
        return getLunName() + "(" + getAccessType() + ")";
    }
}
