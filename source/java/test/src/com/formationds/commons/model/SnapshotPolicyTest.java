/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

public class SnapshotPolicyTest {
//  @Test
//  public void createTest() {
//    try {
//      final Long id = 1L;
//      final SnapshotPolicy policy = new SnapshotPolicy();
//      final Long retention = System.currentTimeMillis() / 1000;
//
//      policy.setId( id );
//      policy.setName( String.format( "snapshot policy name %d", 1 ) );
//      policy.setRecurrenceRule( new RecurrenceRule( "RRULE:FREQ=MONTHLY;INTERVAL=2;BYDAY=TU" ) );
//      policy.setRetention( retention );
//
//      System.out.println( policy.toString() );
//
//      Assert.assertEquals( "Id don't match", policy.getId(), id );
//      Assert.assertEquals( "Name don't match", policy.getName(),
//                           "snapshot policy name 1" );
//      Assert.assertEquals( "RRULE:FREQ=MONTHLY;INTERVAL=2;BYDAY=TU",
//                           policy.getRecurrenceRule()
//                                 .toString() );
//      Assert.assertEquals( "Retention don't match", policy.getRetention(),
//                           retention );
//
//      System.out.println( ObjectModelHelper.toJSON( policy ) );
//    } catch( ParseException e ) {
//      Assert.fail( e.getMessage() );
//      e.printStackTrace();
//    }
//  }

  //  private static final String JSON_1 =
//    "{\n" +
//      "  \"id\": 1,\n" +
//      "  \"name\": \"snapshot policy name 1\",\n" +
//      "  \"recurrenceRule\": {\n" +
//      "    \"frequency\": \"DAILY\",\n" +
//      "    \"count\": 10\n" +
//      "  },\n" +
//      "  \"retention\": 1412281425\n" +
//      "}";
  private static final String JSON_2 =
    "{\"name\":\"1898484965023354702_DAILY\",\"recurrenceRule\":{\"FREQ\":\"DAILY\"},\"retention\":604800}";

//  @Test
//  public void jsonTest() {
//    System.out.println( "POLICY JSON_1::" +
//                          ObjectModelHelper.toObject( JSON_1,
//                                                      SnapshotPolicy.class ) );
//    System.out.println( "POLICY JSON_2::" +
//                        ObjectModelHelper.toObject( JSON_2,
//                                                    SnapshotPolicy.class ) );
//  }
}
