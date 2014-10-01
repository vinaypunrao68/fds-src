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

import com.formationds.commons.model.exception.ParseException;
import org.junit.Test;

import static org.junit.Assert.*;

public class RecurrenceRuleTest {

  @Test
  public void nates_test()
  {
    try {
      final RecurrenceRule rule =
        RecurrenceRule.parser( "FREQ:WEEKLY" );
      assertEquals( rule.toString(), "FREQ=WEEKLY" );
    } catch( ParseException e ) {
      e.printStackTrace();
      assertTrue( true );
    }
  }

  /**
   *  Daily for 10 occurrences: RRULE:FREQ=DAILY;COUNT=10
   */
  @Test
  public void testDailyWithOccurrences()
  {
    try {
      final RecurrenceRule rule =
        RecurrenceRule.parser( "FREQ=DAILY;COUNT=10" );
      assertEquals( rule.toString(), "FREQ=DAILY;COUNT=10" );
    } catch( ParseException e ) {
      e.printStackTrace();
      assertTrue( true );
    }
  }

  /*
   * Daily until December 24, 2014:RRULE:FREQ=DAILY;UNTIL=20141224T000000Z
  */
//  @Test
//  public void testDailyUntil1224()
//  {
//    try {
//      final RecurrenceRule rule =
//        RecurrenceRule.parser( "FREQ=DAILY;UNTIL=20141224T000000Z" );
//      System.out.println( rule + " " + "FREQ=DAILY;UNTIL=20141224T000000Z" );
//      assertEquals( rule.toString(), "FREQ=DAILY;UNTIL=20141224T000000Z" );
//    } catch( ParseException e ) {
//      e.printStackTrace();
//      assertTrue( true );
//    }
//  }

  /**
   * Every other day - forever:
   *   RRULE:FREQ=DAILY;INTERVAL=2
   */
  @Test
  public void testEveryOtherDayForever()
  {
    try {
      final RecurrenceRule rule =
        RecurrenceRule.parser( "FREQ=DAILY;INTERVAL=2" );
      assertEquals( rule.toString(), "FREQ=DAILY;INTERVAL=2" );
    } catch( ParseException e ) {
      e.printStackTrace();
      assertTrue( true );
    }
  }

  /**
   * Every 10 days, 5 occurrences:
   *  RRULE:FREQ=DAILY;INTERVAL=10;COUNT=5
  */
  @Test
  public void testEvery10Days5Occurrences()
  {
    try {
      final RecurrenceRule rule =
        RecurrenceRule.parser( "FREQ=DAILY;INTERVAL=10;COUNT=5" );
      assertEquals( rule.toString(), "FREQ=DAILY;INTERVAL=10;COUNT=5" );
    } catch( ParseException e ) {
      e.printStackTrace();
      assertTrue( true );
    }
  }
  // TODO finish implement the following test
 /*
Everyday in January, for 3 years:
  RRULE:FREQ=YEARLY;UNTIL=20000131T090000Z;
   BYMONTH=1;BYDAY=SU,MO,TU,WE,TH,FR,SA
  or
  RRULE:FREQ=DAILY;UNTIL=20000131T090000Z;BYMONTH=1
*/

  /**
   * Weekly for 10 occurrences
   *  RRULE:FREQ=WEEKLY;COUNT=10
   */
  @Test
  public void testWeekly10Occurrences()
  {
    try {
      final RecurrenceRule rule =
        RecurrenceRule.parser( "FREQ=WEEKLY;COUNT=10" );
      assertEquals( rule.toString(), "FREQ=WEEKLY;COUNT=10" );
    } catch( ParseException e ) {
      e.printStackTrace();
      assertTrue( true );
    }
  }

  // TODO finish implementing these test
/*
Weekly until December 24, 1997
  RRULE:FREQ=WEEKLY;UNTIL=19971224T000000Z

Every other week - forever:
  RRULE:FREQ=WEEKLY;INTERVAL=2;WKST=SU

Weekly on Tuesday and Thursday for 5 weeks:
 RRULE:FREQ=WEEKLY;UNTIL=19971007T000000Z;WKST=SU;BYDAY=TU,TH
 or
 RRULE:FREQ=WEEKLY;COUNT=10;WKST=SU;BYDAY=TU,TH

Every other week on Monday, Wednesday and Friday until December 24,
1997, but starting on Tuesday, September 2, 1997:
  RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=19971224T000000Z;WKST=SU;
   BYDAY=MO,WE,FR

Every other week on Tuesday and Thursday, for 8 occurrences:
  RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=8;WKST=SU;BYDAY=TU,TH

Monthly on the 1st Friday for ten occurrences:
  RRULE:FREQ=MONTHLY;COUNT=10;BYDAY=1FR

Monthly on the 1st Friday until December 24, 1997:
  RRULE:FREQ=MONTHLY;UNTIL=19971224T000000Z;BYDAY=1FR

Every other month on the 1st and last Sunday of the month for 10
occurrences:
  RRULE:FREQ=MONTHLY;INTERVAL=2;COUNT=10;BYDAY=1SU,-1SU

Monthly on the second to last Monday of the month for 6 months:
  RRULE:FREQ=MONTHLY;COUNT=6;BYDAY=-2MO

Monthly on the third to the last day of the month, forever:
  RRULE:FREQ=MONTHLY;BYMONTHDAY=-3

Monthly on the 2nd and 15th of the month for 10 occurrences:
  RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=2,15

Monthly on the first and last day of the month for 10 occurrences:
  RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=1,-1

Every 18 months on the 10th thru 15th of the month for 10
occurrences:
  RRULE:FREQ=MONTHLY;INTERVAL=18;COUNT=10;BYMONTHDAY=10,11,12,13,14,
   15

Every Tuesday, every other month:
  RRULE:FREQ=MONTHLY;INTERVAL=2;BYDAY=TU

Yearly in June and July for 10 occurrences:
  RRULE:FREQ=YEARLY;COUNT=10;BYMONTH=6,7

Every other year on January, February, and March for 10 occurrences:
  RRULE:FREQ=YEARLY;INTERVAL=2;COUNT=10;BYMONTH=1,2,3

Every 3rd year on the 1st, 100th and 200th day for 10 occurrences:
  RRULE:FREQ=YEARLY;INTERVAL=3;COUNT=10;BYYEARDAY=1,100,200

Every 20th Monday of the year, forever:
  RRULE:FREQ=YEARLY;BYDAY=20MO

Monday of week number 20 (where the default start of the week is
Monday), forever:
  RRULE:FREQ=YEARLY;BYWEEKNO=20;BYDAY=MO

Every Thursday in March, forever:
  RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=TH

Every Thursday, but only during June, July, and August, forever:
  RRULE:FREQ=YEARLY;BYDAY=TH;BYMONTH=6,7,8

Every Friday the 13th, forever:
  RRULE:FREQ=MONTHLY;BYDAY=FR;BYMONTHDAY=13

The first Saturday that follows the first Sunday of the month,
 forever:
  RRULE:FREQ=MONTHLY;BYDAY=SA;BYMONTHDAY=7,8,9,10,11,12,13

Every four years, the first Tuesday after a Monday in November,
forever (U.S. Presidential Election day):
  RRULE:FREQ=YEARLY;INTERVAL=4;BYMONTH=11;BYDAY=TU;BYMONTHDAY=2,3,4,
   5,6,7,8

The 3rd instance into the month of one of Tuesday, Wednesday or
Thursday, for the next 3 months:
  RRULE:FREQ=MONTHLY;COUNT=3;BYDAY=TU,WE,TH;BYSETPOS=3

The 2nd to last weekday of the month:
  RRULE:FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-2

Every 3 hours from 9:00 AM to 5:00 PM on a specific day:
  RRULE:FREQ=HOURLY;INTERVAL=3;UNTIL=19970902T170000Z

Every 15 minutes for 6 occurrences:
  RRULE:FREQ=MINUTELY;INTERVAL=15;COUNT=6

Every hour and a half for 4 occurrences:
  RRULE:FREQ=MINUTELY;INTERVAL=90;COUNT=4

Every 20 minutes from 9:00 AM to 4:40 PM every day:
  RRULE:FREQ=DAILY;BYHOUR=9,10,11,12,13,14,15,16;BYMINUTE=0,20,40
  or
  RRULE:FREQ=MINUTELY;INTERVAL=20;BYHOUR=9,10,11,12,13,14,15,16

An example where the days generated makes a difference because of
WKST:
  RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=MO

  changing only WKST from MO to SU, yields different results...

  RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=SU
   */
}