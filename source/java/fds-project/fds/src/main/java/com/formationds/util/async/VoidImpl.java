package com.formationds.util.async;

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
