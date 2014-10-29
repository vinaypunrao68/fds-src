/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.helper.ObjectModelHelper;
import org.junit.Assert;
import org.junit.Test;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class SnapshotPolicyTest {
    private static final List<String> RRULES = new ArrayList<>();

    static {
        RRULES.add( "RRULE:FREQ=WEEKLY;BYHOUR=0;BYDAY=WE" );
        RRULES.add( "RRULE:FREQ=MONTHLY;INTERVAL=2;BYDAY=TU" );
        RRULES.add( "RRULE:FREQ=DAILY;UNTIL=19971224T000000Z" );
        RRULES.add( "RRULE:FREQ=MONTHLY;INTERVAL=2;COUNT=10;BYDAY=1SU,-1SU" );
        RRULES.add( "RRULE:FREQ=MONTHLY;INTERVAL=18;COUNT=10;BYMONTHDAY=10,11,12,13,14,15" );
    }

    @Test
    public void test() {
        try {
            for( final String rrule : RRULES ) {
                final SnapshotPolicy policy = new SnapshotPolicy();
                final Long retention = System.currentTimeMillis() / 1000;

                policy.setRecurrenceRule( rrule );

                policy.setId( new Random().nextLong() );
                policy.setName( String.format( "snapshot::policy::%s",
                                               policy.getRecurrenceRule()
                                                     .getFrequency() ) );
                policy.setRetention( retention );

                System.out.println( "toString:: " + policy.toString() );

                final String json = ObjectModelHelper.toJSON( policy );
                System.out.println( "toJson:: " + json );

                System.out.println( "toObject::" +
                                        ObjectModelHelper.toObject( json,
                                                                    SnapshotPolicy.class )
                                                         .toString() );
            }
        } catch( ParseException e ) {
            Assert.fail( e.getMessage() );
            e.printStackTrace();
        }
    }
}
