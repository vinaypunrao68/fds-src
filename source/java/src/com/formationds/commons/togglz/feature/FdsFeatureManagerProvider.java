/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature;

import com.formationds.commons.togglz.FdsTogglzConfig;
import org.togglz.core.manager.FeatureManager;
import org.togglz.core.manager.FeatureManagerBuilder;

/**
 * @author ptinius
 */
public enum FdsFeatureManagerProvider {

    ;

    private static FeatureManager instance = null;

    /**
     * singleton instance of FdsFeatureManagerProvider
     */
    public static FeatureManager getFeatureManager()
    {
        if( instance == null )
        {
            final FdsTogglzConfig config = new FdsTogglzConfig();

            instance =
                new FeatureManagerBuilder().featureEnum( config.getFeatureClass() )
                                           .stateRepository( config.getStateRepository() )
                                           .userProvider( config.getUserProvider() )
                                           .build();
        }

        return instance;
    }
}
