/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import java.util.HashSet;
import java.util.Set;

/**
 * @author ptinius
 */
public class LUN
{
    public enum AccessType { RW, RO }

    public static class Builder {
        private AccessType accessType;
        private String lunName;
        private Set<IncomingUser> incomingUsers = new HashSet<>( );
        private Set<OutgoingUser> outgoingUsers = new HashSet<>( );

        protected AccessType getAccessType( ) { return accessType; }
        protected String getLunName( ) { return lunName; }
        protected Set<IncomingUser> getIncomingUsers() { return incomingUsers; }
        protected Set<OutgoingUser> getOutgoingUsers() { return outgoingUsers; }

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

        public Builder withIncomingUser( final IncomingUser incomingUser )
        {
            incomingUsers.add( incomingUser );
            return this;
        }

        public Builder withOutgoingUser( final OutgoingUser outgoingUser )
        {
            outgoingUsers.add( outgoingUser );
            return this;
        }

        public LUN build() {
            return new LUN( getLunName(), getAccessType(), getIncomingUsers(), getOutgoingUsers() );
        }
    }

    private final AccessType accessType;
    private final String lunName;

    private final Set<IncomingUser> incomingUsers = new HashSet<>( );
    private final Set<OutgoingUser> outgoingUsers = new HashSet<>( );

    /**
     * @param lunName the name of the logical unit number
     * @param accessType the access control
     */
    protected LUN( final String lunName,
                   final AccessType accessType,
                   final Set<IncomingUser> incomingUsers,
                   final Set<OutgoingUser> outgoingUsers )
    {
        this.lunName = lunName;
        this.accessType = accessType;
        this.incomingUsers.addAll( incomingUsers );
        this.outgoingUsers.addAll( outgoingUsers );
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

    /**
     * @return Returns the incoming users for this Logical Unit Number
     */
    public Set<IncomingUser> getIncomingUsers( )
    {
        return incomingUsers;
    }

    /**
     * @return Returns the outgoing users for this Logical Unit Number
     */
    public Set<OutgoingUser> getOutgoingUsers( )
    {
        return outgoingUsers;
    }

    @Override
    public String toString( )
    {
        return getLunName() + "(" + getAccessType() + ")";
    }
}
