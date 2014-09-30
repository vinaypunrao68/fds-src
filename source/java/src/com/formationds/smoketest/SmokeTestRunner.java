package com.formationds.smoketest;

import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;

/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

public class SmokeTestRunner {
    public static void main(String[] args) throws Exception {
        new BlockJUnit4ClassRunner(SmokeTest.class) {
            @Override
            protected void runChild(FrameworkMethod method, RunNotifier notifier) {
                System.out.println("Running test " + method.getName());
                super.runChild(method, notifier);
            }
        }.run(new RunNotifier() {
            @Override
            public void fireTestFailure(Failure failure) {
                System.out.println("Test failed: " + failure.getMessage());
                System.out.println("Trace: " + failure.getTrace());
                System.exit(-1);
            }
        });
    }

}
