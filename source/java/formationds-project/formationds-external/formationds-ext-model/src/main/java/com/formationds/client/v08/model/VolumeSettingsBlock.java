/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

/**
 *
 */
public class VolumeSettingsBlock extends VolumeSettings {
    private Size capacity;
    private Size blockSize;

    public VolumeSettingsBlock() { }

    public VolumeSettingsBlock( Size capacity ) {
    	this( capacity, null );
    }
    
    public VolumeSettingsBlock( long capacity, long blockSize, SizeUnit unit ) {
        this( Size.of( capacity, unit ), Size.of( blockSize, unit ));
    }

    public VolumeSettingsBlock( Size capacity, Size blockSize ) {
        super( );

        this.capacity = capacity;
        this.blockSize = blockSize;
        this.type = VolumeType.BLOCK;
    }

    /**
     *
     * @return a copy of the settings
     */
    public VolumeSettingsBlock newSettingsFrom() {
        return new VolumeSettingsBlock( getCapacity(), getBlockSize() );
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
        if ( !(o instanceof com.formationds.client.v08.model.VolumeSettingsBlock) ) { return false; }
        final com.formationds.client.v08.model.VolumeSettingsBlock
            block = (com.formationds.client.v08.model.VolumeSettingsBlock) o;
        return Objects.equals( capacity, block.capacity ) &&
               Objects.equals( blockSize, block.blockSize );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), capacity, blockSize );
    }
}
