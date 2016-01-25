/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.util.Configuration;

import java.io.Serializable;

/**
 * @author ptinius
 */
public class SingletonConfiguration implements Serializable {

    private static final long serialVersionUID = 493525199822398655L;

    /**
     * private constructor
     */
    private SingletonConfiguration() {
    }

    private static class SingletonConfigurationHolder {
        protected static final SingletonConfiguration INSTANCE = new SingletonConfiguration();
    }

    /**
     *
     * @return the platform configuration
     */
    public static ParsedConfig getPlatformConfig() {
        return getConfig().getPlatformConfig();
    }

    /**
     *
     * @return the Configuration
     */
    public static Configuration getConfig() {
        return instance().doGetConfig();
    }

    /**
     *
     * @param key
     * @param defaultValue
     *
     * @return the integer value for the specified key, or the default value if not set
     */
    public static int getIntValue(String key, int defaultValue) {
        return getConfig().getPlatformConfig().defaultInt( key, defaultValue );
    }

    /**
     *
     * @param key
     * @param defaultValue
     *
     * @return the string value for the specified key, or the default value if not set
     */
    public static String getStringValue(String key, String defaultValue) {
        return getConfig().getPlatformConfig().defaultString( key, defaultValue );
    }

    /**
     *
     * @param key
     * @param defaultValue
     *
     * @return the boolean value for the specified key or the default value if not set
     */
    public static boolean getBooleanValue(String key, boolean defaultValue) {
        return getConfig().getPlatformConfig().defaultBoolean( key, defaultValue );
    }

    /**
     * @return Returns SingletonConfiguration instance
     */
    public static SingletonConfiguration instance() {
        return SingletonConfigurationHolder.INSTANCE;
    }

    /**
     * @return Returns the {@link com.formationds.om.helper.SingletonConfiguration
     */
    protected Object readResolve() {
        return instance();
    }

    private Configuration config;

    /**
     * @return Returns the {@link com.formationds.util.Configuration}
     */
    private Configuration doGetConfig() {
        return config;
    }

    /**
     * @param config the {@link com.formationds.util.Configuration}
     */
    // TODO: should really only be set by Main.  Consider moving to om package and making
    // package-private
    public void setConfig( final Configuration config ) {
        this.config = config;
    }
}
