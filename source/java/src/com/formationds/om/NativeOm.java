package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.io.File;

public class NativeOm {
    public static native void init(String[] args);

    public static void startOm(String[] args) {
        String gluePath = new File("./liborchmgr.so").getAbsolutePath();
        System.load(gluePath);
        new Thread(() -> init(args), "native-om").start();
    }
}
