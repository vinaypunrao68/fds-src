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
import org.junit.Assert;
import org.junit.Test;

public class MetadataTest
{
  @Test
  public void createTest()
  {
    final Metadata md = ObjectFactory.createMetadata();

    final long time = System.currentTimeMillis();

    md.setKey( "Volume Value Key" );
    md.setTimestamp( time );
    md.setValue( "Volume Value Key Value" );
    md.setVolume( "Volume Name" );

    System.out.println( md.toString() );

    Assert.assertEquals( "Key don't match", md.getKey(), "Volume Value Key" );
    Assert.assertEquals( "Timestamp don't match", md.getTimestamp(), time );
    Assert.assertEquals( "Value don't match",
                         md.getValue(),
                         "Volume Value Key Value" );
    Assert.assertEquals( "Name don't match", md.getVolume(), "Volume Name" );

    System.out.println( ObjectModelHelper.toJSON( md ) );
  }
}