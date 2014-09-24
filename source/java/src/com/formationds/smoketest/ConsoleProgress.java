package com.formationds.smoketest;

import java.util.function.Function;
import java.util.function.IntUnaryOperator;

/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

public class ConsoleProgress implements IntUnaryOperator {
    private int totalEvents;

    public ConsoleProgress(String name, int totalEvents) {
        this.totalEvents = totalEvents;
        System.out.print("    " + name);
        System.out.flush();
    }

    @Override
    public int applyAsInt(int integer) {
        if (integer == totalEvents - 1) {
            System.out.println("  done.");
            System.out.flush();
        } else {
            System.out.print(".");
            System.out.flush();
        }
        return integer;
    }
}
