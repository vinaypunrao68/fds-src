/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;

/**
 * Base volume settings.  At this point this just maintains a VolumeType identifier that can be used
 * when marshalling JSON to identify the specific subclass to load.
 */
abstract public class VolumeSettings {

    private final VolumeType type;
    private VolumeSettings() { type = null; /* hack to make compiler happy */ }
    protected VolumeSettings( VolumeType type ) { this.type = type; }
    public VolumeType getVolumeType() { return this.type; }

    /**
     * Create a copy of the settings based on the current settings
     * @return a copy of the settings
     */
    abstract public VolumeSettings newSettingsFrom();

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
