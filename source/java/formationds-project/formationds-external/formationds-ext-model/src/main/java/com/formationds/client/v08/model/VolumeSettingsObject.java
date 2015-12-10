/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

/**
 *
 */
public class VolumeSettingsObject extends VolumeSettings {
    private Size maxObjectSize;

    public VolumeSettingsObject()
    {
        this( Size.mb( 2 ) );
    }

    public VolumeSettingsObject( long maxObjectSize, SizeUnit unit ) {
        this(Size.of( maxObjectSize, unit ));
    }
    public VolumeSettingsObject( Size maxObjectSize ) {
        super( );
        this.maxObjectSize = maxObjectSize;
        this.type = VolumeType.OBJECT;
    }

    public VolumeSettingsObject newSettingsFrom() {
        return new VolumeSettingsObject( getMaxObjectSize() );
    }

    public Size getMaxObjectSize( ) {
        return maxObjectSize;
    }

    public void setMaxObjectSize( Size maxObjectSize ) {
        this.maxObjectSize = maxObjectSize;
    }

    public void setMaxObjectSize( long size,  SizeUnit unit ) {
        setMaxObjectSize( Size.of( size, unit ) );
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof com.formationds.client.v08.model.VolumeSettingsObject) ) { return false; }
        final com.formationds.client.v08.model.VolumeSettingsObject
            object = (com.formationds.client.v08.model.VolumeSettingsObject)o;
        return Objects.equals( maxObjectSize, object.maxObjectSize );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), maxObjectSize );
    }
}
