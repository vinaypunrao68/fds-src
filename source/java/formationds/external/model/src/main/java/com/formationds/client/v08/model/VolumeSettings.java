/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

abstract public class VolumeSettings {

    /**
     *
     */
    public static class SystemVolumeSettings extends VolumeSettings {
        public SystemVolumeSettings() {
            super( VolumeType.SYSTEM );
        }
    }

    /**
     *
     */
    public static class BlockVolumeSettings extends VolumeSettings {
        private final Size capacity;
        private final Size blockSize;

        public BlockVolumeSettings( Size capacity, Size blockSize ) {
            super( VolumeType.BLOCK );
            this.capacity = capacity;
            this.blockSize = blockSize;
        }

        public Size getCapacity() {
            return capacity;
        }

        public Size getBlockSize() {
            return blockSize;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof BlockVolumeSettings) ) { return false; }
            final BlockVolumeSettings block = (BlockVolumeSettings) o;
            return Objects.equals( capacity, block.capacity ) &&
                   Objects.equals( blockSize, block.blockSize );
        }

        @Override
        public int hashCode() {
            return Objects.hash( super.hashCode(), capacity, blockSize );
        }
    }

    /**
     *
     */
    public static class ObjectVolumeSettings extends VolumeSettings {
        private final Size maxObjectSize;
        
        public ObjectVolumeSettings( Size maxObjectSize ) {
            super( VolumeType.OBJECT );
            this.maxObjectSize = maxObjectSize;
        }

        public Size getMaxObjectSize() {
            return maxObjectSize;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof ObjectVolumeSettings) ) { return false; }
            final ObjectVolumeSettings object = (ObjectVolumeSettings)o;
            return Objects.equals( maxObjectSize, object.maxObjectSize );
        }

        @Override
        public int hashCode() {
            return Objects.hash( super.hashCode(), maxObjectSize );
        }
    }

    private final VolumeType type;
    protected VolumeSettings( VolumeType type ) { this.type = type; }
    public VolumeType getVolumeType() { return this.type; }

    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof VolumeSettings) ) { return false; }
        final VolumeSettings that = (VolumeSettings)o;
        return Objects.equals( type, that.type );
    }

    @Override
    public int hashCode() {
        return Objects.hash( type );
    }

}
