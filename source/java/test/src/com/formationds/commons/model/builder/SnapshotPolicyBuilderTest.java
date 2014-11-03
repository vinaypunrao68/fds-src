/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

public class SnapshotPolicyBuilderTest {
  private static final Long EXCEPTED_ID = 1234L;
  private static final String EXPECTED_NAME =
    SnapshotPolicyBuilderTest.class.getSimpleName();
  private static final Long EXPECTED_RETENTION = 9863784L;

//  @Test
//  public void test()
//  {
//    try {
//      final SnapshotPolicy policy =
//        new SnapshotPolicyBuilder().withId( EXCEPTED_ID )
//                                   .withName( EXPECTED_NAME )
//                                   .withRecurrenceRule(
//                                     new RecurrenceRule(
//                                       "RRULE:FREQ=YEARLY;INTERVAL=3;COUNT=10;" +
//                                         "BYYEARDAY=1,100,200" ) )
//                                       .withRetention( EXPECTED_RETENTION )
//                                       .build();
//
//      Assert.assertEquals( policy.getId(), EXCEPTED_ID );
//      Assert.assertEquals( policy.getName(), EXPECTED_NAME );
//      Assert.assertEquals(
//        policy.getRecurrenceRule(),
//        "RRULE:FREQ=YEARLY;INTERVAL=3;COUNT=10;BYYEARDAY=1,100,200" );
//      Assert.assertEquals( policy.getRetention(), EXPECTED_RETENTION );
//    } catch( ParseException e ) {
//      Assert.fail( e.getMessage() );
//      e.printStackTrace();
//    }
//  }
}