package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

public class FlickrStreamTest {
    //@Test
    public void testIterator() {
        FlickrStream executor = new FlickrStream("arduino robot", 500);
        while (executor.hasNext()) {
            System.out.println(executor.next().getUrl());
        }
    }

    @Test
    public void makeJunitHappy() {

    }
}
