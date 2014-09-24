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
import org.junit.Test;

import java.util.Date;

public class RecurrenceRuleBuilderTest {

  @Test
  public void test()
  {
    final RecurrenceRule rrule =
      new RecurrenceRuleBuilder().withFrequency( iCalFields.DAILY )
                                 .withCount( 10 )
                                 .build();
    final SnapshotPolicy policy =
      new SnapshotPolicyBuilder().withId( 1 )
                                 .withName( "snapshot policy - " +
                                              RecurrenceRuleBuilderTest.class.getSimpleName() )
                                 .withRecurrenceRule( rrule )
                                 .withRetention( 34567789 )
                                 .build();
    System.out.println( policy );

    final RecurrenceRule rrule1 =
      new RecurrenceRuleBuilder().withFrequency( iCalFields.DAILY )
                                 .withUntil( new Date() )
                                 .build();

    final SnapshotPolicy policy1 =
      new SnapshotPolicyBuilder().withId( 1 )
                                 .withName( "snapshot policy - " +
                                              RecurrenceRuleBuilderTest.class.getSimpleName() )
                                 .withRecurrenceRule( rrule1 )
                                 .withRetention( 34567789 )
                                 .build();
    System.out.println( policy1 );
  }
}