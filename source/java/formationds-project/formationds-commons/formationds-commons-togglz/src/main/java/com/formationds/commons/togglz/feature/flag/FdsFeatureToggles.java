/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature.flag;

import com.formationds.commons.togglz.feature.FdsFeatureManagerProvider;
import org.togglz.core.Feature;
import org.togglz.core.annotation.Label;
import org.togglz.core.repository.FeatureState;

/**
 * feature togglz annotation
 *
 * Togglz annotation are used to group features together in "feature groups".
 *
 * @author ptinius
 */
public enum FdsFeatureToggles implements Feature {

    @Label( "OM Service Proxy Feature" )
    OM_SERVICE_PROXY("fds.feature_toggle.om.enable_java_om_svc_proxy"),

    @Label( "REST 0.7 API Implementation" )
    REST_07( "fds.feature_toggle.om.rest_07_feature" ),

    @Label( "Enable old firebreak query behavior." )
    FS_2660_METRIC_FIREBREAK_EVENT_QUERY( "fds.feature_toggle.om.fs_2660_metric_firebreak_event_query" ),

    @Label( "Are we using the new super block?, i.e. partition scheme" )
    NEW_SUPERBLOCK( "fds.feature_toggle.pm.use_new_superblock" ),

    @Label( "Enable InfluxDB Write batching" )
    INFLUX_WRITE_BATCHING( "fds.feature_toggle.om.enable_influxdb_write_batch" ),

    @Label( "Enable web logging request wrapper")
    WEB_LOGGING_REQUEST_WRAPPER( "fds.feature_toggle.common.enable_web_logging_request_wrapper" );

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
