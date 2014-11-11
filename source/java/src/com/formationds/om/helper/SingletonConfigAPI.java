/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import com.formationds.xdi.ConfigurationApi;

import java.io.Serializable;

/**
 * @author ptinius
 */
@SuppressWarnings( "NonSerializableFieldInSerializableClass" )
public class SingletonConfigAPI
    implements Serializable {
    private static final long serialVersionUID = 8183577476145998036L;

    private static class SingletonConfigAPIHolder {
        protected static final SingletonConfigAPI INSTANCE =
            new SingletonConfigAPI();
    }

    /**
     * @return Returns the singleton instance
     */
    public static SingletonConfigAPI instance() {
        return SingletonConfigAPIHolder.INSTANCE;
    }

    /**
     * @return Returns the singleton instance
     */
    protected Object readResolve() {
        return instance();
    }

    private ConfigurationApi api;

    /**
     * @return Returns {@link com.formationds.xdi.ConfigurationApi}
     */
    public ConfigurationApi api() {
        return api;
    }

    /**
     * @param api the {@link com.formationds.xdi.ConfigurationApi}
     */
    public void api( final ConfigurationApi api ) {
        this.api = api;
    }
}
