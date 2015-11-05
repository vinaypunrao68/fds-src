/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import com.formationds.client.v08.model.AbstractResource;

import java.util.HashSet;
import java.util.Set;

/**
 * @author ptinius
 */
public class Target
   extends AbstractResource<Long>
{
    public static class Builder
    {
        private Set<LUN> luns = new HashSet<>( );
        private Set<Initiator> initiators = new HashSet<>( );
        private Set<Credentials> incomingUsers = new HashSet<>( );
        private Set<Credentials> outgoingUsers = new HashSet<>( );

        protected Set<LUN> getLuns( )
        {
            return luns;
        }

        public Builder withLun( final LUN lun )
        {
            this.luns.add( lun );
            return this;
        }

        public Builder withLuns( Set<LUN> luns )
        {
            this.luns.clear();
            this.luns.addAll( luns );
            return this;
        }

        protected Set<Initiator> getInitiators( )
        {
            return initiators;
        }

        public Builder withInitiator( final Initiator initiator )
        {
            this.initiators.add( initiator );
            return this;
        }

        public Builder withInitiators( final Set<Initiator> initiators )
        {
            this.initiators.clear();
            this.initiators.addAll( initiators );
            return this;
        }

        protected Set<Credentials> getIncomingUsers( )
        {
            return incomingUsers;
        }

        public Builder withIncomingUser( final Credentials incomingUser )
        {
            this.incomingUsers.add( incomingUser );
            return this;
        }

        public Builder withIncomingUsers( final Set<Credentials> incomingUsers )
        {
            this.incomingUsers.clear();
            this.incomingUsers = incomingUsers;
            return this;
        }

        protected Set<Credentials> getOutgoingUsers( )
        {
            return outgoingUsers;
        }

        public Builder withOutgoingUser( final Credentials outgoingUser )
        {
            this.outgoingUsers.add( outgoingUser );
            return this;
        }

        public Builder withOutgoingUsers( final Set<Credentials> outgoingUsers )
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

            if( !getInitiators().isEmpty() )
            {
                target.setInitiators( getInitiators() );
            }

            return target;
        }
    }

    private Set<LUN> luns = new HashSet<>( );
    private Set<Initiator> initiators = new HashSet<>( );
    private Set<Credentials> incomingUsers = new HashSet<>( );
    private Set<Credentials> outgoingUsers = new HashSet<>( );

    /**
     * @param luns the set of logical unit number {@link LUN} for this target
     */
    public Target( final Set<LUN> luns )
    {
        this.luns.addAll( luns );
    }

    /**
     * @return Returns the set of logical unit numbers (@link LUN} for this target
     */
    public Set<LUN> getLuns( )
    {
        return luns;
    }

    /**
     * @param luns the set of logical unit numbers (@link LUN} for this target
     */
    public void setLuns( final Set<LUN> luns )
    {
        this.luns = luns;
    }

    /**
     * @return Returns the set of initiators (@link Initiator} for this target
     */
    public Set<Initiator> getInitiators( )
    {
        return initiators;
    }

    /**
     * @param initiators the set of initiators (@link Initiator} for this target
     */
    public void setInitiators( final Set<Initiator> initiators )
    {
        this.initiators = initiators;
    }

    /**
     * @return Returns the set of incoming users {@link Credentials} for this target
     */
    public Set<Credentials> getIncomingUsers( )
    {
        return incomingUsers;
    }

    /**
     * @param incomingUsers the set of incoming users {@link Credentials} for this target
     */
    public void setIncomingUsers( final Set<Credentials> incomingUsers )
    {
        this.incomingUsers = incomingUsers;
    }

    /**
     * @return Returns the set of outgoing users {@link Credentials} for this target
     */
    public Set<Credentials> getOutgoingUsers( )
    {
        return outgoingUsers;
    }

    /**
     * @param outgoingUsers the set of outgoing users {@link Credentials} for this target
     */
    public void setOutgoingUsers( final Set<Credentials> outgoingUsers )
    {
        this.outgoingUsers = outgoingUsers;
    }
}
