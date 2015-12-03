/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * @author ptinius
 */
public class LUN
{
    public enum AccessType { RW, RO }

    public static class Builder {
        private AccessType accessType = AccessType.RW;
        private String lunName;
        private List<Initiator> initiators = new ArrayList<>( );

        protected AccessType getAccessType( ) { return accessType; }
        protected String getLunName( ) { return lunName; }
        protected List<Initiator> getInitiators( ) { return initiators; }

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

        public Builder withInitiator( final Initiator initiator )
        {
            this.initiators.add( initiator );
            return this;
        }

        public LUN build() {
            return new LUN( getLunName(), getAccessType(), getInitiators() );
        }
    }

    private final AccessType accessType;
    private final String lunName;
    private final List<Initiator> initiators;

    /**
     * @param lunName the name of the logical unit number
     * @param accessType the access control
     */
    protected LUN( final String lunName,
                   final AccessType accessType,
                   final List<Initiator> initiators )
    {
        this.lunName = lunName;
        this.accessType = accessType;
        this.initiators = initiators;
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
     * @return Returns the iscsi initiators
     */
    public List<Initiator> getInitiators( ) { return initiators; }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof LUN ) ) return false;
        final LUN lun = ( LUN ) o;
        return getAccessType( ) == lun.getAccessType( ) &&
            Objects.equals( getLunName( ), lun.getLunName( ) ) &&
            Objects.equals( getInitiators(), lun.getInitiators() );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( getAccessType( ), getLunName( ) );
    }

    @Override
    public String toString( )
    {
        return getLunName() + "(" + getAccessType() + ") " + getInitiators();
    }
}
