/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.v08.model.iscsi.Target;

/**
 * @author ptinius
 */
public class VolumeSettingsISCSI
    extends VolumeSettings
{
    public static class Builder
    {
        private Target target;

        public Target getTarget( )
        {
            return target;
        }

        public Builder withTarget( final Target target )
        {
            this.target = target;
            return this;
        }

        public VolumeSettingsISCSI build()
        {
            return new VolumeSettingsISCSI( getTarget() );
        }
    }

    private Target target;

    public VolumeSettingsISCSI( )
    {
        super( VolumeType.ISCSI );
    }

    public VolumeSettingsISCSI( final Target target )
    {
        super( VolumeType.ISCSI  );

        this.target = target;
    }

    public Target getTarget( )
    {
        return target;
    }

    public void setTarget( final Target target )
    {
        this.target = target;
    }

    /**
     * Create a copy of the settings based on the current settings
     *
     * @return a copy of the settings
     */
    @Override
    public VolumeSettings newSettingsFrom( )
    {
        return new VolumeSettingsISCSI( getTarget() );
    }
}
