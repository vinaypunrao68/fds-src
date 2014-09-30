/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
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
                           "FREQ=DAILY;COUNT=10");
      Assert.assertEquals("Retention don't match", policy.getRetention(),
                          retention);

      System.out.println(ObjectModelHelper.toJSON(policy));
    }
}
