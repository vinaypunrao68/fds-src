package com.formationds.smoketest;

import java.util.function.IntUnaryOperator;

/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

public class ConsoleProgress implements IntUnaryOperator {
    private final int totalEvents;
    private int module;

    public ConsoleProgress(String name, int totalEvents, int module) {
        this.totalEvents = totalEvents;
        this.module = module;
        System.out.print("    " + name);
        System.out.flush();
    }

    public ConsoleProgress(String name, int totalEvents) {
        this(name, totalEvents, 1);
    }

    @Override
    public int applyAsInt(int integer) {
        if (integer == totalEvents - 1) {
            System.out.println("  done.");
            System.out.flush();
        } else {
            if (integer % module == 0) {
                System.out.print(".");
                System.out.flush();
            }
        }
        return integer;
    }
}
