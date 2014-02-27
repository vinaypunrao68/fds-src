package com.formationds.samplewebapp;

import baggage.hypertoolkit.WebApp;

import java.lang.management.ManagementFactory;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Main extends WebApp {
    public Main() {
        super(() -> new Hello(), "web");
//        new ServerSetup().setUp();
        System.out.println("Current PID: " + ManagementFactory.getRuntimeMXBean().getName());
    //    Fds nativeApi = new NativeApi();
     //  route("put", () -> new PutTest(nativeApi));
    }

    public static void main(String[] args) throws Exception {
        new Main().start(4242);
    }
}
