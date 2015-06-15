package com.formationds;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

public class CalculatorTest {
    @Test
    public void testAdd() {
        Calculator calculator = new Calculator();
        int result = calculator.add(2, 8);
        System.out.println(result);
    }
}
