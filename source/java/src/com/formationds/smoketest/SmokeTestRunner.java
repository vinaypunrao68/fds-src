package com.formationds.smoketest;

import com.google.common.collect.Lists;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;

/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

public class SmokeTestRunner {
    public static void main(String[] args) throws Exception {
        Lists.newArrayList(HdfsSmokeTest.class, S3SmokeTest.class)
                .forEach(k -> runTest(k));
        System.out.flush();
        System.out.println("Smoke tests done.");
        System.exit(0);
    }

    private static void runTest(final Class klass) {
        BlockJUnit4ClassRunner runner = null;
        try {
            runner = new BlockJUnit4ClassRunner(klass) {
                @Override
                protected void runChild(FrameworkMethod method, RunNotifier notifier) {
                    System.out.println("Running test " + klass.getName() + "." + method.getName());
                    super.runChild(method, notifier);
                }
            };
        } catch (InitializationError initializationError) {
            System.out.println("Error initializing tests");
            initializationError.printStackTrace();
            System.exit(-1);
        }

        runner.run(new RunNotifier() {
            @Override
            public void fireTestFailure(Failure failure) {
                System.out.println("Test failed: " + failure.getMessage());
                System.out.println("Trace: " + failure.getTrace());
                System.exit(-1);
            }
        });
    }

}
