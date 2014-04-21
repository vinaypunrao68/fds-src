package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


import java.io.File;

public class NativeAm {
    public static native void init(String[] args);

    public static void startAm(String[] args) {
        String gluePath = new File("./libaccessmgr.so").getAbsolutePath();
        System.load(gluePath);
        new Thread(() -> init(args)).start();
    }
}
