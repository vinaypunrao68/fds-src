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

package com.formationds.commons.model;

import com.formationds.commons.model.builder.RecurrenceRuleBuilder;
import com.formationds.commons.model.helper.ObjectModelHelper;
import org.junit.Assert;
import org.junit.Test;

public class SnapshotPolicyTest {
    @Test
    public void createTest() {
      final Long id = 1L;
        final SnapshotPolicy policy = new SnapshotPolicy();
      final Long retention = System.currentTimeMillis() / 1000;

      policy.setId( id );
      policy.setName(String.format("snapshot policy name %d", 1));
      policy.setRecurrenceRule(
        new RecurrenceRuleBuilder().withFrequency( "DAILY" )
                                   .withCount( 10 )
                                   .build() );
      policy.setRetention( retention );

      System.out.println(policy.toString());

      Assert.assertEquals("Id don't match", policy.getId(), id );
      Assert.assertEquals("Name don't match", policy.getName(),
                          "" + "snapshot policy name 1");
      Assert.assertEquals( policy.getRecurrenceRule().toString(),
                           "[ FREQ=DAILY;COUNT=10; ]");
      Assert.assertEquals("Retention don't match", policy.getRetention(),
                          retention);

      System.out.println(ObjectModelHelper.toJSON(policy));
    }

  private static final String JSON_1 =
    "{\n" +
      "  \"id\": 1,\n" +
      "  \"name\": \"snapshot policy name 1\",\n" +
      "  \"recurrenceRule\": {\n" +
      "    \"frequency\": \"DAILY\",\n" +
      "    \"count\": 10\n" +
      "  },\n" +
      "  \"retention\": 1412281425\n" +
      "}";
  private static final String JSON_2 =
    "{\"name\":\"1898484965023354702_DAILY\",\"recurrenceRule\":{\"FREQ\":\"DAILY\"},\"retention\":604800}";

  @Test
  public void jsonTest()
  {
    final SnapshotPolicy policy = ObjectModelHelper.toObject( JSON_2, SnapshotPolicy.class );

    System.out.println( "POLICY::" + policy );
  }
}
