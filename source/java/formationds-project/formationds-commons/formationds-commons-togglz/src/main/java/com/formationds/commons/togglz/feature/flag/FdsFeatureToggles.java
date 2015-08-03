/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature.flag;

import com.formationds.commons.togglz.feature.FdsFeatureManagerProvider;
import org.togglz.core.Feature;
import org.togglz.core.annotation.Label;
import org.togglz.core.repository.FeatureState;

/**
 * @author ptinius
 */
public enum FdsFeatureToggles
  implements Feature {

  /*
   * feature togglz annotation
   *
   * Togglz annotation are used to group features together in "feature groups".
   */

    @Label( "REST 0.7 API Implementation" )
    REST_07( "fds.feature_toggle.om.rest_07_feature" ),

    @Label( "Enable old firebreak query behavior." )
    FS_2660_METRIC_FIREBREAK_EVENT_QUERY( "fds.feature_toggle.om.fs_2660_metric_firebreak_event_query" );

    /**
     * @return Returns {@code true} if the feature associated with {@code this}
     * is enabled
     */
    public boolean isActive() {

        return FdsFeatureManagerProvider.getFeatureManager()
                                        .isActive( this );

    }

    /**
     * @param featureState the {@code boolean} flag used to set the feature availability.
     */
    public void state( final boolean featureState ) {
        FdsFeatureManagerProvider.getFeatureManager()
                                 .setFeatureState( new FeatureState( this )
                                                       .setEnabled( featureState ) );
    }

    private final String fdsName;

    FdsFeatureToggles( final String fdsName ) {
        this.fdsName = fdsName;
    }

    public String fdsname()
    {
        return this.fdsName;
    }
}
