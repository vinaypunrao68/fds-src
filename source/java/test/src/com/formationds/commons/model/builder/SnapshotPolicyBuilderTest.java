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

import com.formationds.commons.model.RecurrenceRule;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.type.iCalFields;
import org.junit.Assert;
import org.junit.Test;

public class SnapshotPolicyBuilderTest {
  private static final String EXPECTED_FREQ_NAME = iCalFields.DAILY.name();
  private static final iCalFields EXPECTED_FREQ_ENUM = iCalFields.DAILY;
  private static final long EXCEPTED_ID = 1234;
  private static final String EXPECTED_NAME =
    SnapshotPolicyBuilderTest.class.getSimpleName();
  private static final int EXPECTED_COUNT = 10;
  private static final long EXPECTED_RETENTION = 9863784;

  @Test
  public void test()
  {
    final RecurrenceRule rrule =
      new RecurrenceRuleBuilder().withFrequency( EXPECTED_FREQ_NAME )
                                 .withCount( EXPECTED_COUNT )
                                 .build();

    Assert.assertEquals( rrule.getFrequency(), EXPECTED_FREQ_ENUM );
    Assert.assertEquals( rrule.getCount(), EXPECTED_COUNT );

    final SnapshotPolicy policy =
      new SnapshotPolicyBuilder().withId( EXCEPTED_ID )
                                 .withName( EXPECTED_NAME )
                                 .withRecurrenceRule( rrule )
                                 .withRetention( EXPECTED_RETENTION ).build();

    Assert.assertEquals( policy.getId(), EXCEPTED_ID );
    Assert.assertEquals( policy.getName(), EXPECTED_NAME );
    Assert.assertEquals( policy.getRecurrenceRule(), rrule );
    Assert.assertEquals( policy.getRetention(), EXPECTED_RETENTION );
  }
}