package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import junit.framework.TestCase;

public class NativeApiTest extends TestCase {
    public void testInit() throws Exception {
        new NativeApi();
        NativeApi.startOm();
        Thread.sleep(1000);
    }
}
