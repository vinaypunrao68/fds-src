/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.snmp;

import org.junit.Assert;
import org.junit.Test;

public class FDSOidTest {

    @Test
    public void test() {

        Assert.assertTrue( "FDS enterprise OID don't match",
                           FDSOid.FDS_OID
                               .format()
                               .equals( "1.3.6.1.4.1.50000" ) );
        Assert.assertTrue( "DS product OID don't match",
                           FDSOid.DS_OID
                               .format()
                               .equals( "1.3.6.1.4.1.50000.1" ) );
        Assert.assertTrue( "Event Time OID don't match",
                           FDSOid.EVENT_TIME_OID
                               .format()
                               .equals( "1.3.6.1.4.1.50000.1.1" ) );
        Assert.assertTrue( "Event service name OID don't match",
                           FDSOid.EVENT_SERVICE_NAME_OID
                               .format()
                               .equals( "1.3.6.1.4.1.50000.1.2" ) );
        Assert.assertTrue( "Event type OID don't match",
                           FDSOid.EVENT_TYPE_OID
                               .format()
                               .equals( "1.3.6.1.4.1.50000.1.3" ) );
        Assert.assertTrue( "Event category OID don't match",
                           FDSOid.EVENT_CATEGORY_OID
                               .format()
                               .equals( "1.3.6.1.4.1.50000.1.4" ) );
        Assert.assertTrue( "Event severity OID don't match",
                           FDSOid.EVENT_SEVERITY_OID
                               .format()
                               .equals( "1.3.6.1.4.1.50000.1.5" ) );
        Assert.assertTrue( "Event message OID don't match",
                           FDSOid.EVENT_MESSAGE_OID
                                 .format()
                                 .equals( "1.3.6.1.4.1.50000.1.6" ) );

    }

}