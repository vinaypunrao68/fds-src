/*
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import com.formationds.apis.ConfigurationService;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.protocol.commonConstants;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

/**
 *
 */
public class ConfigServiceClientFactory {
    protected static final Logger LOG = Logger.getLogger(ThriftClientFactory.class);

    /**
     * Create with default settings.  Must use #getClient(host, port) to access the client
     *
     * @see #newConfigService(String, Integer, int, int, int)
     */
    public static ThriftClientFactory<ConfigurationService.Iface> newConfigService() {
        return newConfigService(null, null,
                                ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                ThriftClientFactory.DEFAULT_MIN_IDLE,
                                ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     * @see #newConfigService(String, Integer, int, int, int)
     */
    public static ThriftClientFactory<ConfigurationService.Iface> newConfigService(String host, Integer port) {
        return newConfigService(host, port,
                                ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                ThriftClientFactory.DEFAULT_MIN_IDLE,
                                ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     * Create a new ConfigurationService.Iface client factory.  The host and port are optional and if provided the
     * ThriftClientFactory#getClient() api will return a connection to that default client host and port.
     * <p/>
     * Otherwise, clients must specify a host and port using ThriftClientFactory#getClient(host, port).
     *
     * @param host the optional default thrift server host.
     * @param port the optional default thrift server port.  If a host is specified, then the port is required.
     * @param maxPoolSize
     * @param minIdle
     * @param softMinEvictionIdleTimeMillis
     *
     * @return a ThriftClientFactory for the ConfigurationService.Iface with the specified default host and port.
     */
    public static ThriftClientFactory<ConfigurationService.Iface> newConfigService(String host, Integer port,
                                                                                    int maxPoolSize,
                                                                                    int minIdle,
                                                                                    int softMinEvictionIdleTimeMillis) {
        /**
         * FEATURE TOGGLE: enable multiplexed services
         * Tue Feb 23 15:04:17 MST 2016
         */
        if ( FdsFeatureToggles.THRIFT_MULTIPLEXED_SERVICES.isActive() ) {

            // service.client will use multiplexed protocol
            return new ThriftClientFactory.Builder<>(ConfigurationService.Iface.class)
                       .withHostPort(host, port)
                       .withPoolConfig(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis)
                       .withThriftServiceName( commonConstants.CONFIGURATION_SERVICE_NAME )
                       .withClientFactory(ConfigurationService.Client::new)
                       .build();
        }

        // service.client will use binary protocol
        return new ThriftClientFactory.Builder<>(ConfigurationService.Iface.class)
                   .withHostPort(host, port)
                   .withPoolConfig(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis)
                   .withClientFactory(ConfigurationService.Client::new)
                   .build();
    }

    /*
     * prevent instantiation
     */
    private ConfigServiceClientFactory() {}
}
