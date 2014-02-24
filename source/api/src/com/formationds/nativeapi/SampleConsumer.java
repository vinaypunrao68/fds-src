package com.formationds.nativeapi;

import java.util.function.Consumer;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class SampleConsumer implements Consumer<Integer> {
    @Override
    public void accept(Integer integer) {
        System.out.println("SampleConsumer got called");
    }

    @Override
    public Consumer<Integer> andThen(Consumer<? super Integer> after) {
        return (Consumer<Integer>) after;
    }
}
