package com.formationds.samplewebapp;

import baggage.hypertoolkit.WebApp;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Main extends WebApp {
    public Main() {
        super(() -> new Hello(), "web");
    }

    public static void main(String[] args) throws Exception {
        new Main().start(4242);
    }
}
