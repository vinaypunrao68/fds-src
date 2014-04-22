package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.function.Consumer;
import java.util.function.Supplier;

public class DemoRunnerTest {
    public void testApi() throws Exception {
        int readThreads = 3;
        int writeThreads = 3;
        String searchExpression = "hello";
        Consumer<ImageResource> writer = (x) -> System.out.println(x);
        Supplier<ImageResource> reader = () -> new ImageResource("42", "http://static.fjcdn.com/pictures/fear_9b10c3_2486227.jpg");

        DemoRunner demoRunner = new DemoRunner(searchExpression, reader, writer, readThreads, writeThreads);
        demoRunner.start();
    }
}
