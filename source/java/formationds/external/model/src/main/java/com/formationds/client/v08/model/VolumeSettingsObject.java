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

    public VolumeSettingsObject() { super(VolumeType.OBJECT); }
    public VolumeSettingsObject( long maxObjectSize, SizeUnit unit ) {
        this(Size.of( maxObjectSize, unit ));
    }
    public VolumeSettingsObject( Size maxObjectSize ) {
        super( VolumeType.OBJECT );
        this.maxObjectSize = maxObjectSize;
    }

    public Size getMaxObjectSize() {
        return maxObjectSize;
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
