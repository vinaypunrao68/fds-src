/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.v08.model.nfs.NfsClients;
import com.formationds.client.v08.model.nfs.NfsOptions;

import java.util.Objects;

/**
 * @author ptinius
 */
public class VolumeSettingsNfs
    extends VolumeSettingsObject
{
    public static class Builder
    {
        private Size maxObjectSize = Size.mb( 2 );
        public Size getMaxObjectSize() { return maxObjectSize; }
        public Builder withMaxObjectSize( final Size maxObjectSize )
        {
            this.maxObjectSize = maxObjectSize;
            return this;
        }

        private String options = "";
        public Builder withOptions( final NfsOptions options )
        {
            this.options = options.getOptions();
            return this;
        }

        private String clients = "*";
        public Builder withClient( final NfsClients clients )
        {
            this.clients = clients.getPattern();
            return this;
        }

        public VolumeSettingsNfs build()
        {
            return new VolumeSettingsNfs( this );
        }
    }

    private final String options;
    private final String clients;

    VolumeSettingsNfs( final Builder builder )
    {
        super( );

        setMaxObjectSize( builder.maxObjectSize );
        this.options = builder.options;
        this.clients = builder.clients;

        this.type = VolumeType.NFS;
    }

    /**
     * @param maxObjectSize the max object size
     * @param clients the nfs clients
     * @param options the nfs options
     */
    public VolumeSettingsNfs( final Size maxObjectSize,
                              final NfsClients clients,
                              final NfsOptions options )
    {
        super( );

        setMaxObjectSize( maxObjectSize );
        this.options = options.getOptions();
        this.clients = clients.getPattern();

        this.type = VolumeType.NFS;
    }

    public VolumeSettingsNfs( final Size maxObjectSize,
                              final String clients,
                              final String options )
    {
        super( );

        setMaxObjectSize( maxObjectSize );
        this.options = options;
        this.clients = clients;

        this.type = VolumeType.NFS;
    }

    /**
     * @return Returns set of NFS options
     */
    public String getOptions( ) { return options; }

    /**
     * @return Returns set of NFS clients
     */
    public String getClients( ) { return clients; }

    /**
     * Create a copy of the settings based on the current settings
     *
     * @return a copy of the settings
     */
    @Override
    public VolumeSettingsNfs newSettingsFrom( )
    {
        return new VolumeSettingsNfs( getMaxObjectSize(), getClients(), getOptions() );
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof VolumeSettingsNfs ) ) return false;
        final VolumeSettingsNfs that = ( VolumeSettingsNfs ) o;
        return Objects.equals( getOptions( ), that.getOptions( ) ) &&
            Objects.equals( getClients( ), that.getClients( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( super.hashCode( ), getOptions( ), getClients( ) );
    }
}
