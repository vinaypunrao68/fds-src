package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.async.AsyncResourcePool;

public class VoidImpl implements AsyncResourcePool.Impl<Void> {

    @Override
    public Void construct() throws Exception {
        return null;
    }

    @Override
    public void destroy(Void elt) {

    }

    @Override
    public boolean isValid(Void elt) {
        return true;
    }
}
