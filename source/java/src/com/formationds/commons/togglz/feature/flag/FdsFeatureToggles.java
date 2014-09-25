/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature.flag;

import com.formationds.commons.togglz.FdsFeatureManagerProvider;
import com.formationds.commons.togglz.feature.annotation.Canned;
import com.formationds.commons.togglz.feature.annotation.JAASUserProvider;
import com.formationds.commons.togglz.feature.annotation.Snapshot;
import com.formationds.commons.togglz.feature.annotation.WebServlets;
import org.togglz.core.Feature;
import org.togglz.core.annotation.Label;

/**
 * @author ptinius
 */
public enum FdsFeatureToggles
  implements Feature
{
  @Label( "Use Canned Data Results (TEST ONLY)" )
  @Canned
  USE_CANNED,

  @Label( "Web Servlet Endpoint Feature" )
  @WebServlets
  WEB_SERVLETS,

  @Label( "Looking up of \"current user\" using JAAS AccessControlContext." )
  @JAASUserProvider
  JAAS_USER,

  @Label( "Snapshot Feature" )
  @Snapshot
  SNAPSHOT_ENDPOINT;

  /**
   * @return Returns {@code true} if the feature associated with {@code this} is enabled
   */
  public boolean isActive()
  {
    return FdsFeatureManagerProvider.getFeatureManager().isActive( this );
  }
}
