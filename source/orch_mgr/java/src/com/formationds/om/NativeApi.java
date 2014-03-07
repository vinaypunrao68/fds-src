package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.MutableAcceptor;

import java.io.File;

public class NativeApi {
    static {
        String gluePath = new File("./liborchmgr.so").getAbsolutePath();
        System.load(gluePath);
    }

    public static native void init(String[] args);
    public static native void listNodes(MutableAcceptor<String> acceptor);

    public static void startOm(String[] args) {
        new Thread(() -> init(args)).start();
    }
}
