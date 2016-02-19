/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.v08.model.iscsi.Target;

import java.util.Objects;

/**
 * @author ptinius
 */
public class VolumeSettingsISCSI
    extends VolumeSettingsBlock
{
    public static class Builder
    {
        public Target getTarget( ) { return target; }
        public Size getCapacity( ) { return capacity; }
        public Size getBlockSize( ) { return blockSize; }
        public SizeUnit getUnit( ) { return unit; }

        public Builder withTarget( final Target target )
        {
            this.target = target;
            return this;
        }

        public Builder withCapacity( final Size capacity )
        {
            this.capacity = capacity;
            return this;
        }

        public Builder withBlockSize( final Size blockSize )
        {
            this.blockSize = blockSize;
            return this;
        }

        public Builder withUnit( final SizeUnit unit )
        {
            this.unit = unit;
            return this;
        }

        public VolumeSettingsISCSI build( )
        {
            if( getCapacity() == null && getBlockSize() == null && getUnit() == null )
            {
                return new VolumeSettingsISCSI( getTarget( ) );
            }
            else if( getUnit() == null && getCapacity() != null && getBlockSize() == null )
            {
                return new VolumeSettingsISCSI( getCapacity(), getTarget( ) );
            }
            else if( getUnit() == null && getCapacity() != null && getBlockSize() != null )
            {
                return new VolumeSettingsISCSI( getCapacity(), getBlockSize(), getTarget( ) );
            }

            return new VolumeSettingsISCSI( getCapacity().getValue().longValue(),
                                            getBlockSize().getValue().longValue(),
                                            getUnit(),
                                            getTarget( ) );
        }

        private Target target;
        private Size capacity = null;
        private Size blockSize = null;
        private SizeUnit unit = null;
    }

    private final Target target;

    public VolumeSettingsISCSI( final Target target )
    {
        this.target = target;
        this.type = VolumeType.ISCSI;
    }

    public VolumeSettingsISCSI( final Size capacity, final Target target )
    {
        super( capacity );
        this.target = target;
        this.type = VolumeType.ISCSI;
    }

    public VolumeSettingsISCSI( final long capacity, final long blockSize,
                                final SizeUnit unit, final Target target )
    {
        super( capacity, blockSize, unit );
        this.target = target;
        this.type = VolumeType.ISCSI;
    }

    public VolumeSettingsISCSI( final Size capacity, final Size blockSize, final Target target )
    {
        super( capacity, blockSize );
        this.target = target;
        this.type = VolumeType.ISCSI;
    }

    /**
     * @return Return {@link Target}
     */
    public Target getTarget( )
    {
        return target;
    }

    /**
     * Create a copy of the settings based on the current settings
     *
     * @return a copy of the settings
     */
    @Override
    public VolumeSettingsISCSI newSettingsFrom( )
    {
        return new VolumeSettingsISCSI( target );
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof VolumeSettingsISCSI ) ) return false;
        final VolumeSettingsISCSI that = ( VolumeSettingsISCSI ) o;
        return Objects.equals( getTarget( ), that.getTarget( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( super.hashCode( ), getTarget( ) );
    }
}
