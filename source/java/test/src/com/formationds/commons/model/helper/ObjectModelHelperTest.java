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

package com.formationds.commons.model.helper;

import com.formationds.commons.model.AuthenticatedUser;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.builder.AuthenticatedUserBuilder;
import com.formationds.commons.model.builder.RecurrenceRuleBuilder;
import com.formationds.commons.model.builder.SnapshotPolicyBuilder;
import com.formationds.commons.model.type.Feature;
import org.junit.Assert;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class ObjectModelHelperTest {

  @Test
  public void test() {
    final List<Feature> features = new ArrayList<>();
    features.add( Feature.SYS_MGMT );
    features.add( Feature.USER_MGMT );

    final AuthenticatedUser user =
      new AuthenticatedUserBuilder().withFeatures( features )
                                    .withToken( "1209809FABCD" )
                                    .withUserId( 11234L )
                                    .withUsername( "Three Little Pigs" )
                                    .build();

    final AuthenticatedUser user1 =
      ObjectModelHelper.toObject( ObjectModelHelper.toJSON( user ),
                                  AuthenticatedUser.class );

    Assert.assertEquals( user.getUserId(), user1.getUserId() );
    Assert.assertEquals( user.getUsername(), user1.getUsername() );
    Assert.assertEquals( user.getToken(), user1.getToken() );
    Assert.assertEquals( user.getFeatures(), user1.getFeatures() );
  }

  @Test
  public void jsonTest()
  {
    final SnapshotPolicy expected =
      new SnapshotPolicyBuilder().withId( 2 )
                                 .withName( "snapshot policy name #2" )
                                 .withRecurrenceRule(
                                   new RecurrenceRuleBuilder().withFrequency( "DAILY" )
                                                              .withCount( 10 )
                                                              .build() )
                                 .withRetention( 34567 )
                                 .build();

    final SnapshotPolicy actual =
      ObjectModelHelper.toObject( ObjectModelHelper.toJSON( expected ),
                                                            SnapshotPolicy.class );

    Assert.assertEquals( expected.getId(), actual.getId() );
    Assert.assertEquals( expected.getName(), actual.getName() );
    Assert.assertEquals( expected.getRecurrenceRule().getCount(),
                         actual.getRecurrenceRule().getCount() );
    Assert.assertEquals( expected.getRecurrenceRule().getFrequency(),
                         actual.getRecurrenceRule().getFrequency() );
    Assert.assertEquals( expected.getRetention(), actual.getRetention() );
  }
}