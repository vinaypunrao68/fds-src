/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

/**
 * Represents a system volume.  System volumes currently have no configuration
 * and cannot be modified.
 */
public class VolumeSettingsSystem extends VolumeSettings {
    public static final VolumeSettingsSystem SYSTEM = new VolumeSettingsSystem();
    public VolumeSettingsSystem() {
        super( );

        this.type = VolumeType.SYSTEM;
    }

    public VolumeSettingsSystem newSettingsFrom() { return new VolumeSettingsSystem(); }
}
