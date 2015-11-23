/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.v08.model.nfs.NfsOptionBase;

import java.util.HashSet;
import java.util.Objects;
import java.util.Set;

/**
 * @author ptinius
 */
public class VolumeSettingsNfs
    extends VolumeSettings
{
    public static class Builder
    {
        private Set< NfsOptionBase > options = new HashSet<>( );
        private Set< IPFilter > ipFilters = new HashSet<>( );

        public Set< NfsOptionBase > getOptions( )
        {
            return options;
        }

        public Builder withOption( final NfsOptionBase option )
        {
            options.add( option );
            return this;
        }

        public Builder withOptions( final Set< NfsOptionBase > options )
        {
            this.options.clear();
            this.options.addAll( options );
            return this;
        }

        public Set< IPFilter > getIpFilters( )
        {
            return ipFilters;
        }

        public Builder withIpFilter( final IPFilter ipFilter )
        {
            ipFilters.add( ipFilter );
            return this;
        }

        public Builder withIpFilters( final Set< IPFilter > ipFilters )
        {
            this.ipFilters.clear();
            this.ipFilters.addAll( ipFilters );
            return this;
        }

        public VolumeSettingsNfs build()
        {
            return new VolumeSettingsNfs( getOptions(), getIpFilters() );
        }
    }

    private Set< NfsOptionBase > options = new HashSet<>( );
    private Set<IPFilter> filters = new HashSet<>( );

    public VolumeSettingsNfs( )
    {
        super( );
        this.type = VolumeType.NFS;
    }

    public VolumeSettingsNfs( final Set< NfsOptionBase > options )
    {
        super( );

        this.options = options;
        this.type = VolumeType.NFS;
    }

    public VolumeSettingsNfs( final Set< NfsOptionBase > options, final Set< IPFilter > ipFilters )
    {
        super( );

        setOptions( options );
        setFilters( ipFilters );
    }

    /**
     * @param options the NFS options
     */
    public void setOptions( Set< NfsOptionBase > options )
    {
        this.options = options;
    }

    /**
     * @return Returns set of NFS options
     */
    public Set<NfsOptionBase> getOptions( )
    {
        return options;
    }

    /**
     * @return Returns set of IP addresses to be filtered
     */
    public Set<IPFilter> getFilters( )
    {
        return filters;
    }

    /**
     * @param filters set of IP addresses to be filtered
     */
    public void setFilters( Set<IPFilter> filters )
    {
        this.filters = filters;
    }

    /**
     * Create a copy of the settings based on the current settings
     *
     * @return a copy of the settings
     */
    @Override
    public VolumeSettings newSettingsFrom( )
    {
        return new VolumeSettingsNfs( getOptions(), getFilters() );
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof VolumeSettingsNfs ) ) return false;
        final VolumeSettingsNfs that = ( VolumeSettingsNfs ) o;
        return Objects.equals( getOptions( ), that.getOptions( ) ) &&
            Objects.equals( getFilters( ), that.getFilters( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( super.hashCode( ), getOptions( ), getFilters( ) );
    }
}
