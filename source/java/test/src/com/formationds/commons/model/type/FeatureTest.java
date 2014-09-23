/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.type;

import org.junit.Assert;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertNotNull;

/**
 * @author ptinius
 */
public class FeatureTest {
  private static final List<Feature> ADMIN_EXPECTED = new ArrayList<>( );
  static {
    ADMIN_EXPECTED.add( Feature.SYS_MGMT );
    ADMIN_EXPECTED.add( Feature.USER_MGMT );
    ADMIN_EXPECTED.add( Feature.TENANT_MGMT );
    ADMIN_EXPECTED.add( Feature.VOL_MGMT );
  }

  private static final List<Feature> USER_EXPECTED = new ArrayList<>( );
  static {
    USER_EXPECTED.add( Feature.USER_MGMT );
    USER_EXPECTED.add( Feature.VOL_MGMT );
  }

  @Test
  public void test()
  {
    for( final Feature feature : Feature.values() )
    {
      assertNotNull( feature );
      assertNotNull( feature.name() );
      assertNotNull( feature.getLocalized() );
    }
  }

  @Test
  public void byRoleAdminTest()
  {
    final List<Feature> features = Feature.byRole( IdentityType.ADMIN );
    for( final Feature feature : features )
    {
      if( !ADMIN_EXPECTED.contains( feature ) )
      {
        Assert.fail( "Missing " + feature + " from expected admin feature list " + ADMIN_EXPECTED );
      }
    }
  }

  @Test
  public void byRoleUserTest()
  {
    final List<Feature> features = Feature.byRole( IdentityType.USER );
    for( final Feature feature : features )
    {
      if( !USER_EXPECTED.contains( feature ) )
      {
        Assert.fail( "Missing " + feature + " from expected user feature list " + USER_EXPECTED );
      }
    }
  }
}
