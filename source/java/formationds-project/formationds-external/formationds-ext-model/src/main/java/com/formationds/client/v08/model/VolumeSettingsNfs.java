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
        public Size getMaxObjectSize() { return maxObjectSize; }
        public Builder withMaxObjectSize( final Size maxObjectSize )
        {
            this.maxObjectSize = maxObjectSize;
            return this;
        }

        public Builder withOptions( final NfsOptions options )
        {
            this.options = options.getOptions();
            return this;
        }

        public Builder withClient( final NfsClients clients )
        {
            this.clients = clients.getClient();
            return this;
        }

        public Builder withCapacity( final Size capacity )
        {
            this.capacity = capacity;
            return this;
        }

        public Builder withVolumeName( final String volumeName )
        {
            this.volumeName = volumeName;
            return this;
        }

        public VolumeSettingsNfs build()
        {
            return new VolumeSettingsNfs( this );
        }

        private String options = "";
        private Size maxObjectSize = Size.mb( 1 );
        private Size capacity;
        private String clients = "*";
        private String volumeName = "";
    }

    private final Size capacity;
    private final String options;
    private final String clients;
    private final String volumeName;

    VolumeSettingsNfs( final Builder builder )
    {
        super( );

        setMaxObjectSize( builder.maxObjectSize );
        this.capacity = builder.capacity;
        this.options = builder.options;
        this.clients = builder.clients;

        this.volumeName = builder.volumeName;

        this.type = VolumeType.NFS;
    }

    /**
     * @param maxObjectSize the max object size
     * @param capacity the capacity limit
     * @param clients the nfs clients
     * @param options the nfs options
     */
    public VolumeSettingsNfs( final Size maxObjectSize,
                              final Size capacity,
                              final NfsClients clients,
                              final NfsOptions options,
                              final String volumeName )
    {
        super( );

        setMaxObjectSize( maxObjectSize );
        this.capacity = capacity;
        this.options = options.getOptions();
        this.clients = clients.getClient();

        this.volumeName = volumeName;


        this.type = VolumeType.NFS;
    }

    public VolumeSettingsNfs( final Size maxObjectSize,
                              final Size capacity,
                              final String clients,
                              final String options,
                              final String volumeName )
    {
        super( );

        setMaxObjectSize( maxObjectSize );
        this.capacity = capacity;
        this.options = options;
        this.clients = clients;

        this.volumeName = volumeName;

        this.type = VolumeType.NFS;
    }

    // Network File System mount point
    private static final String NFS_FMT = "/%s";

    /**
     * @return Returns set of NFS options
     */
    public String getOptions( ) { return options; }

    /**
     * @return Returns set of NFS clients
     */
    public String getClients( ) { return clients; }

    /**
     * @return Returns the capacity limit
     */
    public Size getCapacity( ) { return  capacity; }

    /**
     * @return Returns the NFS mount point
     */
    public String getMountPoint( ) { return String.format( NFS_FMT, volumeName ); }

    /**
     * Create a copy of the settings based on the current settings
     *
     * @return a copy of the settings
     */
    @Override
    public VolumeSettingsNfs newSettingsFrom( )
    {
        return new VolumeSettingsNfs( getMaxObjectSize( ),
                                      getCapacity( ),
                                      getClients( ),
                                      getOptions( ),
                                      volumeName );
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
        return Objects.hash( super.hashCode( ), getOptions( ), getClients( ), volumeName );
    }
}
