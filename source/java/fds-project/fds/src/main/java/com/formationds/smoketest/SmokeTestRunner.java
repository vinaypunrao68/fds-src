package com.formationds.smoketest;

import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

public class SmokeTestRunner {

    HashMap<String, Class> map = new HashMap<String, Class>();

    public SmokeTestRunner() {
        addClass(S3SmokeTest.class);
        addClass(S3SmokeTest.V4Auth.class);
        addClass(HdfsSmokeTest.class);
        addClass(AsyncAmTest.class);
        addClass(NfsTest.class);
    }

    void addClass(final Class klass) {
        String parts[]=klass.getName().toLowerCase().split("\\.");
        map.put(parts[parts.length-1], klass);
    }

    void run(String[] args) {

        if (args.length == 0) {
            System.out.println("running all tests");
            map.values().forEach(k -> runTest(k));
        } else {
            for(String arg : args) {
                String parts[] = arg.split("\\.");
                if (parts.length > 1) {
                    String className = parts[0].toLowerCase();
                    String methodName = parts[1];
                    for(String key: map.keySet()) {
                        if (key.startsWith(className)) {
                            System.out.println("running test [" + methodName + "] for " + key);
                            runTest(map.get(key), methodName);
                        }
                    }
                } else {
                    String className = parts[0];
                    for(String key: map.keySet()) {
                        if (key.startsWith(className)) {
                            System.out.println("running all tests : " + className);
                            runTest(map.get(key));
                        }
                    }
                }
            }
        }
    }

    public static void main(String[] args) throws Exception {
        // NOTE: for debug logging recommend passing in a -Dlog4j2.configurationFile=<file>
        // with file containing required log configuration.  Some considerations include:
        // rootLogger = DEBUG
        // com.amazonaws = WARN
        // com.amazonaws.request = DEBUG
        // org.apache.http.wire" = DEBUG

        SmokeTestRunner testRunner = new SmokeTestRunner();
        testRunner.run(args);
        System.out.flush();
        System.out.println("Smoke tests done.");
        System.exit(0);
    }

    public static void runTest(final Class klass) {
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
            System.exit(1);
        }

        runner.run(new RunNotifier() {
            @Override
            public void fireTestFailure(Failure failure) {
                System.out.println("Test failed: " + failure.getMessage());
                System.out.println("Trace: " + failure.getTrace());
                System.exit(1);
            }
        });
    }

    public static void runTest(final Class klass, String methodName) {
        BlockJUnit4ClassRunner runner = null;
        try {
            runner = new BlockJUnit4ClassRunner(klass) {
                    @Override
                    protected List<FrameworkMethod> computeTestMethods() {
                        try {
                            for(Method m : klass.getMethods()) {
                                if (m.getName().equalsIgnoreCase(methodName)) {
                                    return Arrays.asList(new FrameworkMethod(m));
                                }
                            }
                            System.out.println("no matching methods : " + methodName);
                        } catch (Exception e) {
                            throw new RuntimeException(e);
                        }
                        return null;
                    }

                    @Override
                    protected void runChild(FrameworkMethod method, RunNotifier notifier) {
                        System.out.println("Running test " + klass.getName() + "." + method.getName());
                        super.runChild(method, notifier);
                    }
                };
        } catch (InitializationError initializationError) {
            System.out.println("Error initializing tests");
            initializationError.printStackTrace();
            System.exit(1);
        }

        runner.run(new RunNotifier() {
            @Override
            public void fireTestFailure(Failure failure) {
                System.out.println("Test failed: " + failure.getMessage());
                System.out.println("Trace: " + failure.getTrace());
                System.exit(1);
            }
        });
    }
}
