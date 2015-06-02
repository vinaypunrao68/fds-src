/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.util.Objects;

abstract public class VolumeSettings {

    /**
     *
     */
    public static class Sys extends VolumeSettings {
        public Sys() {
            super( VolumeType.SYSTEM );
        }
    }

    /**
     *
     */
    public static class Blk extends VolumeSettings {
        private final Size capacity;
        private final Size blockSize;

        public Blk( Size capacity, Size blockSize ) {
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
            if ( !(o instanceof Blk) ) { return false; }
            final Blk blk = (Blk) o;
            return Objects.equals( capacity, blk.capacity ) &&
                   Objects.equals( blockSize, blk.blockSize );
        }

        @Override
        public int hashCode() {
            return Objects.hash( super.hashCode(), capacity, blockSize );
        }
    }

    /**
     *
     */
    public static class Obj extends VolumeSettings {
        private final Size maxObjectSize;

        public Obj( Size maxObjectSize ) {
            super( VolumeType.OBJECT );
            this.maxObjectSize = maxObjectSize;
        }

        public Size getMaxObjectSize() {
            return maxObjectSize;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof Obj) ) { return false; }
            if ( !super.equals( o ) ) { return false; }
            final Obj that = (Obj) o;
            return Objects.equals( maxObjectSize, that.maxObjectSize );
        }

        @Override
        public int hashCode() {
            return Objects.hash( super.hashCode(), maxObjectSize );
        }
    }

    private final VolumeType type;
    protected VolumeSettings( VolumeType type ) { this.type = type; }
    public VolumeType getVolumeType() { return this.type; }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof VolumeSettings) ) { return false; }
        final VolumeSettings that = (VolumeSettings) o;
        return Objects.equals( type, that.type );
    }

    @Override
    public int hashCode() {
        return Objects.hash( type );
    }
}
