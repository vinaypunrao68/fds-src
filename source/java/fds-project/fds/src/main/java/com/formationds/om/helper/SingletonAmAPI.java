/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import com.formationds.xdi.AsyncAm;

import java.io.IOException;
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

    private transient AsyncAm api;

    /**
     * @param api the {@link AsyncAm} representing the async AM api
     */
    public void api( final AsyncAm api ) {
        this.api = api;

        try {
            api.start();
        } catch (IOException e) {
            System.out.println(e.toString());
            this.api = null;
        }
    }

    /**
     * @return Returns the {@link AsyncAm} representing the AM api
     */
    public AsyncAm api() {
        return this.api;
    }
}
