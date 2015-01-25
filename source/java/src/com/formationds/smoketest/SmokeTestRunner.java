package com.formationds.smoketest;

import com.google.common.collect.Lists;
import org.apache.log4j.PropertyConfigurator;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;

import java.util.Properties;

/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

public class SmokeTestRunner {
    public static void main(String[] args) throws Exception {
        turnLog4jOff();
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

    public static void turnLog4jOff() {
        Properties properties = new Properties();
        properties.put("log4j.rootCategory", "OFF, console");
        properties.put("log4j.appender.console", "org.apache.log4j.ConsoleAppender");
        properties.put("log4j.appender.console.layout", "org.apache.log4j.PatternLayout");
        properties.put("log4j.appender.console.layout.ConversionPattern", "%-4r [%t] %-5p %c %x - %m%n");
        PropertyConfigurator.configure(properties);
    }

    public static void turnLog4jOn() {
        Properties properties = new Properties();
        properties.put("log4j.rootCategory", "DEBUG, console");
        properties.put("log4j.appender.console", "org.apache.log4j.ConsoleAppender");
        properties.put("log4j.appender.console.layout", "org.apache.log4j.PatternLayout");
        properties.put("log4j.appender.console.layout.ConversionPattern", "%-4r [%t] %-5p %c %x - %m%n");
        PropertyConfigurator.configure(properties);
    }
}
