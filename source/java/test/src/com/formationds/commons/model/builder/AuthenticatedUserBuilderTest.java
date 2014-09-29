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

package com.formationds.commons.model.builder;

import com.formationds.commons.model.AuthenticatedUser;
import com.formationds.commons.model.type.Feature;
import org.junit.Assert;
import org.junit.Test;

import java.util.Arrays;
import java.util.List;
import java.util.UUID;

public class AuthenticatedUserBuilderTest {
  private static final String EXPECTED_USERNAME = "joe-user";
  private static final Long EXPECTED_USERID = 0L;
  private static final String EXPECTED_TOKEN = UUID.randomUUID().toString();
  private static final List<Feature> EXPECTED_FEATURES =
    Arrays.asList( Feature.SYS_MGMT, Feature.TENANT_MGMT );

  @Test
  public void test()
  {
    final AuthenticatedUser authUser =
      new AuthenticatedUserBuilder().withUsername( EXPECTED_USERNAME )
                                    .withUserId( EXPECTED_USERID )
                                    .withToken( EXPECTED_TOKEN )
                                    .withFeatures( EXPECTED_FEATURES )
                                    .build();
    Assert.assertEquals( authUser.getUsername(), EXPECTED_USERNAME );
    Assert.assertEquals( authUser.getUserId(), EXPECTED_USERID );
    Assert.assertEquals( authUser.getToken(), EXPECTED_TOKEN );
    final int size = EXPECTED_FEATURES.size();
    Assert.assertArrayEquals( authUser.getFeatures().toArray( new Feature[ size ] ),
                              EXPECTED_FEATURES.toArray( new Feature[ size ] ) );
  }

}