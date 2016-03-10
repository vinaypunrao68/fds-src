package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.logging.log4j.LogManager;

import java.io.File;

public class NativeOm {
    public static native void init(String[] args);

    public static void startOm(String[] args) {
        String gluePath = new File("./liborchmgr.so").getAbsolutePath();
        System.load(gluePath);
        new Thread(() -> init(args), "native-om").start();

        /*
         * Total hack to allow all the native side interfaces to become ready before hitting them from the java side.
         */
        try {
            Thread.sleep( 5000 );
        } catch (InterruptedException ex) {
            LogManager.getLogger( NativeOm.class.getName( ) )
                         .trace( "sleep interrupted!", ex) ;
        }
    }
}
