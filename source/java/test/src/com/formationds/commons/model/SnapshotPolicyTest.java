/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
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

import com.formationds.commons.model.helper.ObjectModelHelper;
import net.fortuna.ical4j.model.Recur;
import org.junit.Assert;
import org.junit.Test;

import java.text.ParseException;

/**
 * @author ptinius
 */
public class SnapshotPolicyTest
{
  @Test
  public void createTest()
  {
    final SnapshotPolicy policy = ObjectFactory.createSnapshotPolicy();

    try
    {
      final Recur  recu = new Recur( "FREQ=DAILY;UNTIL=19971224T000000Z" );
      final long retention = System.currentTimeMillis() / 1000;
      policy.setId( 1 );
      policy.setName( String.format( "snapshot policy name %d", 1 ) );
      policy.setRecurrenceRule( recu );
      policy.setRetention( retention );

      System.out.println( policy.toString() );

      Assert.assertEquals( "Id don't match", policy.getId(), 1 );
      Assert.assertEquals( "Name don't match", policy.getName(),
                           "" + "snapshot policy name 1" );
      Assert.assertEquals( "RecurrenceRule don't match",
                           policy.getRecurrenceRule()
                                 .toString(),
                           "FREQ=DAILY;UNTIL=19971224T000000Z" );
      Assert.assertEquals( "Retention don't match", policy.getRetention(),
                           retention );

      System.out.println( ObjectModelHelper.toJSON( policy ) );
    }
    catch( ParseException e )
    {
      Assert.fail( e.getMessage() );
    }
  }
}
