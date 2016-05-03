/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import com.formationds.xdi.AuthenticatedXdi;

import java.io.Serializable;

/**
 * @author ptinius
 */
public class SingletonXdi
    implements Serializable {

    private static final long serialVersionUID = -1L;

    /**
     * private constructor
     */
    private SingletonXdi( ) {
    }

    private static class SingletonXdiHolder {
        protected static final SingletonXdi INSTANCE = new SingletonXdi();
    }

    /**
     * @return Returns SingletonXdi instance
     */
    public static SingletonXdi instance( ) {
        return SingletonXdiHolder.INSTANCE;
    }

    /**
     * @return Returns the {@link com.formationds.om.helper.SingletonXdi
     */
    protected Object readResolve( ) {
        return instance();
    }

    private AuthenticatedXdi api;

    /**
     * @return Returns {@link AuthenticatedXdi}
     */
    public AuthenticatedXdi api() {
        return api;
    }

    /**
     * @param api the {@link AuthenticatedXdi}
     */
    public void api(final AuthenticatedXdi api) {
        this.api = api;
    }
}
