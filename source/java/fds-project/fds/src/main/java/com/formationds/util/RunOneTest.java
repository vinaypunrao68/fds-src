package com.formationds.util;

import com.formationds.smoketest.SmokeTestRunner;

public class RunOneTest {
    public static void main(String[] args) throws Exception {
        if (args.length == 0 || args.length > 2) {
            System.out.println("Usage: RunOneTest com.formationds.TestClass");
            System.out.println("   or: RunOneTest com.formationds.TestClass testMethod");
            System.exit(1);
        }
        SmokeTestRunner.turnLog4jOff();

        Class testClass = Class.forName(args[0]);

        if (args.length == 1) {
            SmokeTestRunner.runTest(testClass);
        } else {
            SmokeTestRunner.runTest(testClass, args[1]);
        }
        System.out.println("Done.");
        System.exit(0);
    }
}
