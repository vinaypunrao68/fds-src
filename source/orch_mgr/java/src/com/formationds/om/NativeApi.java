package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.io.File;

public class NativeApi {
    static {
        String gluePath = new File("./orchMgr").getAbsolutePath();
        System.load(gluePath);
    }

    public static native void init();

    public static void startOm() {
        new Thread(() -> init()).start();
    }
}
