/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import com.formationds.client.v08.model.AbstractResource;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * @author ptinius
 */
public class Target
   extends AbstractResource<Long>
{
    public static class Builder
    {
        private List<LUN> luns = new ArrayList<>( );
        private List<Credentials> incomingUsers = new ArrayList<>( );
        private List<Credentials> outgoingUsers = new ArrayList<>( );

        protected List<LUN> getLuns( )
        {
            return luns;
        }

        public Builder withLun( final LUN lun )
        {
            this.luns.add( lun );
            return this;
        }

        public Builder withLuns( List<LUN> luns )
        {
            this.luns.clear();
            this.luns.addAll( luns );
            return this;
        }

        protected List<Credentials> getIncomingUsers( )
        {
            return incomingUsers;
        }

        public Builder withIncomingUser( final Credentials incomingUser )
        {
            this.incomingUsers.add( incomingUser );
            return this;
        }

        public Builder withIncomingUsers( final List<Credentials> incomingUsers )
        {
            this.incomingUsers.clear();
            this.incomingUsers = incomingUsers;
            return this;
        }

        protected List<Credentials> getOutgoingUsers( )
        {
            return outgoingUsers;
        }

        public Builder withOutgoingUser( final Credentials outgoingUser )
        {
            this.outgoingUsers.add( outgoingUser );
            return this;
        }

        public Builder withOutgoingUsers( final List<Credentials> outgoingUsers )
        {
            this.outgoingUsers.clear();
            this.outgoingUsers = outgoingUsers;
            return this;
        }

        public Target build( )
        {
            final Target target = new Target( getLuns() );
            if( !getIncomingUsers().isEmpty() )
            {
                target.setIncomingUsers( getIncomingUsers() );
            }

            if( !getOutgoingUsers().isEmpty() )
            {
                target.setOutgoingUsers( getOutgoingUsers() );
            }

            return target;
        }
    }

    private List<LUN> luns = new ArrayList<>( );
    private List<Credentials> incomingUsers = new ArrayList<>( );
    private List<Credentials> outgoingUsers = new ArrayList<>( );

    /**
     * @param luns the list of logical unit number {@link LUN} for this target
     */
    public Target( final List<LUN> luns )
    {
        this.luns.addAll( luns );
    }

    /**
     * @return Returns the list of logical unit numbers (@link LUN} for this target
     */
    public List<LUN> getLuns( )
    {
        return luns;
    }

    /**
     * @param luns the list of logical unit numbers (@link LUN} for this target
     */
    public void setLuns( final List<LUN> luns )
    {
        this.luns = luns;
    }

    /**
     * @return Returns the list of incoming users {@link Credentials} for this target
     */
    public List<Credentials> getIncomingUsers( )
    {
        return incomingUsers;
    }

    /**
     * @param incomingUsers the list of incoming users {@link Credentials} for this target
     */
    public void setIncomingUsers( final List<Credentials> incomingUsers )
    {
        this.incomingUsers = incomingUsers;
    }

    /**
     * @return Returns the list of outgoing users {@link Credentials} for this target
     */
    public List<Credentials> getOutgoingUsers( )
    {
        return outgoingUsers;
    }

    /**
     * @param outgoingUsers the list of outgoing users {@link Credentials} for this target
     */
    public void setOutgoingUsers( final List<Credentials> outgoingUsers )
    {
        this.outgoingUsers = outgoingUsers;
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof Target ) ) return false;
        if ( !super.equals( o ) ) return false;
        final Target target = ( Target ) o;
        return Objects.equals( getLuns( ), target.getLuns( ) ) &&
            Objects.equals( getIncomingUsers( ), target.getIncomingUsers( ) ) &&
            Objects.equals( getOutgoingUsers( ), target.getOutgoingUsers( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( super.hashCode( ),
                             getLuns( ),
                             getIncomingUsers( ),
                             getOutgoingUsers( ) );
    }
}
