/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.helper.ObjectModelHelper;
import net.fortuna.ical4j.model.Recur;
import org.junit.Assert;
import org.junit.Test;

import java.text.ParseException;

public class SnapshotPolicyTest {
    @Test
    public void createTest() {
        final SnapshotPolicy policy = new SnapshotPolicy();

        try {
            final Recur recu = new Recur("FREQ=DAILY;UNTIL=19971224T000000Z");
            final long retention = System.currentTimeMillis() / 1000;
            policy.setId(1);
            policy.setName(String.format("snapshot policy name %d", 1));
            policy.setRecurrenceRule(recu);
            policy.setRetention(retention);

            System.out.println(policy.toString());

            Assert.assertEquals("Id don't match", policy.getId(), 1);
            Assert.assertEquals("Name don't match", policy.getName(),
                    "" + "snapshot policy name 1");
            Assert.assertEquals("RecurrenceRule don't match",
                    policy.getRecurrenceRule()
                            .toString(),
                    "FREQ=DAILY;UNTIL=19971224T000000Z");
            Assert.assertEquals("Retention don't match", policy.getRetention(),
                    retention);

            System.out.println(ObjectModelHelper.toJSON(policy));
        } catch (ParseException e) {
            Assert.fail(e.getMessage());
        }
    }
}
