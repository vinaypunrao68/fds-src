/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature.flag;

import com.formationds.commons.togglz.feature.FdsFeatureManagerProvider;
import com.formationds.commons.togglz.feature.annotation.Activities;
import com.formationds.commons.togglz.feature.annotation.Firebreak;
import com.formationds.commons.togglz.feature.annotation.Snapshot;
import com.formationds.commons.togglz.feature.annotation.Snmp;
import com.formationds.commons.togglz.feature.annotation.Statistics;
import com.formationds.commons.togglz.feature.annotation.Webkit;
import org.togglz.core.Feature;
import org.togglz.core.annotation.EnabledByDefault;
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

  @EnabledByDefault
  @Label("Snapshot Feature")
  @Snapshot
  SNAPSHOT_ENDPOINT,

  @EnabledByDefault
  @Label("Statistics Feature")
  @Statistics
  STATISTICS_ENDPOINT,

  @EnabledByDefault
  @Label( "Activities Feature" )
  @Activities
  ACTIVITIES_ENDPOINT,

  @EnabledByDefault
  @Label( "Firebreak Event Feature" )
  @Firebreak
  FIREBREAK_EVENT,

  @EnabledByDefault
  @Label("Firebreak Feature")
  @Firebreak
  FIREBREAK,

  @EnabledByDefault
  @Label( "Webkit Feature" )
  @Webkit
  WEB_KIT,

  @Label( "SNMP Feature" )
  @Snmp
  SNMP;

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
}
