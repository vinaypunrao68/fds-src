/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.builder.RecurrenceRuleBuilder;
import com.formationds.commons.model.builder.SnapshotPolicyBuilder;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.iCalFrequency;
import com.formationds.commons.model.type.iCalWeekDays;
import com.formationds.commons.util.Numbers;
import com.formationds.commons.util.WeekDays;
import org.junit.Assert;
import org.junit.Test;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;

public class RecurrenceRuleTest {

    private static final List<String> RRULES = new ArrayList<>();

    static {
        RRULES.add( "RRULE:FREQ=DAILY;" );
        RRULES.add( "RRULE:FREQ=DAILY" );
        // Daily for 10 occurrences
        RRULES.add( "RRULE:FREQ=DAILY;COUNT=10" );
        // Daily until December 24, 1997
        RRULES.add( "RRULE:FREQ=DAILY;UNTIL=19971224T000000Z" );
        // Every other day - forever
        RRULES.add( "RRULE:FREQ=DAILY;INTERVAL=2" );
        // Every 10 days, 5 occurrences:
        RRULES.add( "RRULE:FREQ=DAILY;INTERVAL=10;COUNT=5" );
        // Weekly on Tuesday and Thursday for 5 weeks
        RRULES.add( "RRULE:FREQ=WEEKLY;UNTIL=19971007T000000Z;WKST=SU;BYDAY=TU,TH" );
        RRULES.add( "RRULE:FREQ=WEEKLY;COUNT=10;WKST=SU;BYDAY=TU,TH" );
        // Everyday in January, for 3 years
        RRULES.add( "RRULE:FREQ=YEARLY;UNTIL=20000131T090000Z;" +
                        "BYMONTH=1;BYDAY=SU,MO,TU,WE,TH,FR,SA" );
        // Every other week on Monday, Wednesday and Friday until December 24,
        // 1997, but starting on Tuesday, September 2, 1997
        RRULES.add( "RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=19971224T000000Z;WKST=SU;BYDAY=MO,WE,FR" );
        // Every other week on Tuesday and Thursday, for 8 occurrences
        RRULES.add( "RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=8;WKST=SU;BYDAY=TU,TH" );
        // Monthly on the 1st Friday for ten occurrences
        RRULES.add( "RRULE:FREQ=MONTHLY;COUNT=10;BYDAY=1FR" );
        // Monthly on the 1st Friday until December 24, 1997
        RRULES.add( "RRULE:FREQ=MONTHLY;UNTIL=19971224T000000Z;BYDAY=1FR" );
        // Every other month on the 1st and last Sunday of the month for 10
        // occurrences
        RRULES.add( "RRULE:FREQ=MONTHLY;INTERVAL=2;COUNT=10;BYDAY=1SU,-1SU" );
        // Monthly on the second to last Monday of the month for 6 months
        RRULES.add( "RRULE:FREQ=MONTHLY;COUNT=6;BYDAY=-2MO" );
        // Monthly on the third to the last day of the month, forever
        RRULES.add( "RRULE:FREQ=MONTHLY;BYMONTHDAY=-3" );
        // Monthly on the 2nd and 15th of the month for 10 occurrences
        RRULES.add( "RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=2,15" );
        // Monthly on the first and last day of the month for 10 occurrences
        RRULES.add( "RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=1,-1" );
        // Every 18 months on the 10th thru 15th of the month for 10 occurrences
        RRULES.add( "RRULE:FREQ=MONTHLY;INTERVAL=18;COUNT=10;BYMONTHDAY=10,11,12,13,14,15" );
        // Every Tuesday, every other month
        RRULES.add( "RRULE:FREQ=MONTHLY;INTERVAL=2;BYDAY=TU" );
        // Yearly in June and July for 10 occurrences
        RRULES.add( "RRULE:FREQ=YEARLY;COUNT=10;BYMONTH=6,7" );
        // Every other year on January, February, and March for 10 occurrences
        RRULES.add( "RRULE:FREQ=YEARLY;INTERVAL=2;COUNT=10;BYMONTH=1,2,3" );
        // Every 3rd year on the 1st, 100th and 200th day for 10 occurrences
        RRULES.add( "RRULE:FREQ=YEARLY;INTERVAL=3;COUNT=10;BYYEARDAY=1,100,200" );
        // Every 20th Monday of the year, forever
        RRULES.add( "RRULE:FREQ=YEARLY;BYDAY=20MO" );
        // Monday of week number 20 (where the default start of the week is
        // Monday), forever
        RRULES.add( "RRULE:FREQ=YEARLY;BYWEEKNO=20;BYDAY=MO" );
        // Every Thursday in March, forever
        RRULES.add( "RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=TH" );
        // Every Thursday, but only during June, July, and August, forever
        RRULES.add( "RRULE:FREQ=YEARLY;BYDAY=TH;BYMONTH=6,7,8" );
        // Every Friday the 13th, forever
        RRULES.add( "RRULE:FREQ=MONTHLY;BYDAY=FR;BYMONTHDAY=13" );
        // The first Saturday that follows the first Sunday of the month, forever
        RRULES.add( "RRULE:FREQ=MONTHLY;BYDAY=SA;BYMONTHDAY=7,8,9,10,11,12,13" );
        // Every four years, the first Tuesday after a Monday in November,
        // forever (U.S. Presidential Election day)
        RRULES.add( "RRULE:FREQ=YEARLY;INTERVAL=4;BYMONTH=11;BYDAY=TU;" +
                        "BYMONTHDAY=2,3,4,5,6,7,8" );
        // The 3rd instance into the month of one of Tuesday, Wednesday or
        // Thursday, for the next 3 months
        RRULES.add( "RRULE:FREQ=MONTHLY;COUNT=3;BYDAY=TU,WE,TH;BYSETPOS=3" );
        // The 2nd to last weekday of the month
        RRULES.add( "RRULE:FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-2" );
        // Every 3 hours from 9:00 AM to 5:00 PM on a specific day
        RRULES.add( "RRULE:FREQ=HOURLY;INTERVAL=3;UNTIL=19970902T170000Z" );
        // Every 15 minutes for 6 occurrences
        RRULES.add( "RRULE:FREQ=MINUTELY;INTERVAL=15;COUNT=6" );
        // Every hour and a half for 4 occurrences
        RRULES.add( "RRULE:FREQ=MINUTELY;INTERVAL=90;COUNT=4" );
        // Every 20 minutes from 9:00 AM to 4:40 PM every day
        RRULES.add( "RRULE:FREQ=DAILY;BYHOUR=9,10,11,12,13,14,15,16;BYMINUTE=0,20,40" );
        RRULES.add( "RRULE:FREQ=MINUTELY;INTERVAL=20;BYHOUR=9,10,11,12,13,14,15,16" );
        // An example where the days generated makes a difference because of WKST
        // changing only WKST from MO to SU, yields different results
        RRULES.add( "RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=MO" );
        RRULES.add( "RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=SU" );
    }

    @Test
    public void test() {
        for( final String rrule : RRULES ) {
            try {
                System.out.println( new RecurrenceRule().parser( rrule ) );
            } catch( ParseException e ) {
                Assert.fail( e.getMessage() );
                e.printStackTrace();
            }
        }
    }

    @Test
    public void singleRRuleTest() {
        try {
            final RecurrenceRule rrule =
                new RecurrenceRule()
                    .parser( "RRULE:FREQ=DAILY;BYHOUR=9,10,11,12,13,14,15,16;BYMINUTE=0,20,40" );
            System.out.println( rrule );
        } catch( ParseException e ) {
            Assert.fail( e.getMessage() );
            e.printStackTrace();
        }
    }

    private String create_json =
        "{\"name\":\"8673493746559185022_WEEKLY\",\"recurrenceRule\":{\"FREQ\":\"WEEKLY\",\"BYHOUR\":[4],\"BYDAY\":[\"SU\"]},\"retention\":2678400,\"use\":true,\"displayName\":\"Weekly\"}";
    private String edit_json =
        "{\"name\":\"960842085991640633_MONTHLY\",\"recurrenceRule\":{\"FREQ\":\"MONTHLY\",\"BYHOUR\":[2],\"BYMONTHDAY\":[\"1\"]},\"retention\":31622400,\"id\":2,\"use\":true,\"displayName\":\"Monthly\"}";

    @Test
    public void nates() {
        final SnapshotPolicy create =
            ObjectModelHelper.toObject( create_json,
                                        SnapshotPolicy.class );
        System.out.println( create );

        final SnapshotPolicy edit =
            ObjectModelHelper.toObject( edit_json,
                                        SnapshotPolicy.class );
        System.out.println( edit );
    }

    @Test
    public void json() {
        final Numbers<Integer> BYHOUR = new Numbers<>( );
        BYHOUR.add( 12 );

        final WeekDays<iCalWeekDays> BYDAY = new WeekDays<>( );
        BYDAY.add( iCalWeekDays.FR );

        final RecurrenceRule rrule =
            new RecurrenceRuleBuilder( iCalFrequency.MINUTELY )
                .byHour( BYHOUR )
                .byDay( BYDAY )
                .build();
        final SnapshotPolicy policy =
            new SnapshotPolicyBuilder().withName( "snapshot::policy::" + rrule.getFrequency() )
                                       .withRecurrenceRule( rrule.toString() )
                                       .withRetention( 1234567890L )
                                       .build();
        System.out.println( policy );
        final String json = policy.toJSON();
        System.out.println( json );
        System.out.println( ObjectModelHelper.toObject( json,
                                                        SnapshotPolicy.class )
                                             .toString() );
    }
}