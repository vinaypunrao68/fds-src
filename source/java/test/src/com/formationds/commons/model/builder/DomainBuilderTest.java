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

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Domain;
import org.junit.Assert;
import org.junit.Test;

public class DomainBuilderTest {
  private static final String EXPECTED_DOMAIN = "Joe's Domain";
  private static final Integer EXPECTED_ID = 678;
  private static final String EXPECTED_SITE = "Joe's Site";

  @Test
  public void test() {
    final Domain domain = new DomainBuilder().withDomain( EXPECTED_DOMAIN )
                                             .withId( EXPECTED_ID )
                                             .withSite( EXPECTED_SITE )
                                             .build();

    Assert.assertEquals( domain.getId(), EXPECTED_ID );
    Assert.assertEquals( domain.getSite(), EXPECTED_SITE );
    Assert.assertEquals( domain.getDomain(), EXPECTED_DOMAIN );
  }

}