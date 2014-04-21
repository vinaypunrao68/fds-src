package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.ArrayList;
import java.util.Collection;

public class Container<T> {
    private Collection<T> tees;

    public Container() {
        tees = (Collection<T>) new ArrayList<Integer>().stream()
                .filter(p -> p % 2 == 0)
                .map(p -> p * 2);

    }


}
