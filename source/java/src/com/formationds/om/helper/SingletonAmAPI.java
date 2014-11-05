/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import com.formationds.apis.AmService;

import java.io.Serializable;

/**
 * @author ptinius
 */
public class SingletonAmAPI
    implements Serializable {

    private static final long serialVersionUID = 1388343850213024609L;

    /**
     * private constructor
     */
    private SingletonAmAPI() {
    }

    private static class SingletonAmAPIHolder {
        protected static final SingletonAmAPI INSTANCE = new SingletonAmAPI();
    }

    /**
     * @return Returns SingletonAmAPI instance
     */
    public static SingletonAmAPI instance() {
        return SingletonAmAPIHolder.INSTANCE;
    }

    /**
     * @return Returns
     */
    protected Object readResolve() {
        return instance();
    }

    private transient AmService.Iface api;

    /**
     * @param api the {@link AmService.Iface} representing the AM api
     */
    public void api( final AmService.Iface api ) {
        this.api = api;
    }

    /**
     * @return Returns the {@link AmService.Iface} representing the AM api
     */
    public AmService.Iface api() {
        return this.api;
    }
}
