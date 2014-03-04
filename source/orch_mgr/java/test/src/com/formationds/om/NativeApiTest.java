package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import junit.framework.TestCase;

public class NativeApiTest extends TestCase {
    public void testInit() throws Exception {
        new NativeApi();
        NativeApi.init();
        Thread.sleep(10000);
        //MutableAcceptor<String> acceptor = new MutableAcceptor<>();
        //NativeApi.listNodes(acceptor);
        //System.out.println("Done listing nodes");
        //Thread.sleep(1000);
    }
}
