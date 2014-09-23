/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
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
