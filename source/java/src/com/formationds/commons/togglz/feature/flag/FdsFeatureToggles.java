/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature.flag;

import com.formationds.commons.togglz.feature.FdsFeatureManagerProvider;
import com.formationds.commons.togglz.feature.annotation.Snmp;
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
