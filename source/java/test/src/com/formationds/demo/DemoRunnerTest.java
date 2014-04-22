package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class DemoRunnerTest {
//    @Test
    public void testApi() throws Exception {
        int readThreads = 3;
        int writeThreads = 3;
        String searchExpression = "france";
        ImageWriter writer = mock(ImageWriter.class);
        ImageReader reader = mock(ImageReader.class);
        when(reader.readOne()).thenReturn(new ImageResource("foo", "http://localhost/poop.jpg"));
        DemoRunner demoRunner = new DemoRunner(searchExpression, reader, writer, readThreads, writeThreads);
        demoRunner.start();

        for (int i = 0; i < 10; i++) {
            Thread.sleep(1000);
            System.out.println(demoRunner.peekWriteQueue().getUrl());
            System.out.println(demoRunner.peekReadQueue().getUrl());
        }
        demoRunner.tearDown();
    }
}

