/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;

import java.io.Serializable;

/**
 * @author ptinius
 */
public class SingletonLegacyConfig
    implements Serializable {

    private static final long serialVersionUID = -1L;

    /**
     * private constructor
     */
    private SingletonLegacyConfig( ) {
    }

    private static class SingletonLegacyConfigHolder {
        protected static final SingletonLegacyConfig INSTANCE = new SingletonLegacyConfig();
    }

    /**
     * @return Returns SingletonLegacyConfig instance
     */
    public static SingletonLegacyConfig instance( ) {
        return SingletonLegacyConfigHolder.INSTANCE;
    }

    /**
     * @return Returns the {@link com.formationds.om.helper.SingletonLegacyConfig
     */
    protected Object readResolve( ) {
        return instance();
    }

    private FDSP_ConfigPathReq.Iface api;

    /**
     * @return Returns {@link FDSP_ConfigPathReq.Iface}
     */
    public FDSP_ConfigPathReq.Iface api() {
        return api;
    }

    /**
     * @param api the {@link FDSP_ConfigPathReq.Iface}
     */
    public void api( final FDSP_ConfigPathReq.Iface api ) {
        this.api = api;
    }
}
