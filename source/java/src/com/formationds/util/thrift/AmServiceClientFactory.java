/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.util.thrift;

import com.formationds.apis.AmService;
import com.formationds.apis.AsyncAmServiceRequest;
import com.formationds.apis.RequestId;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.UUID;

/**
 *
 */
public class AmServiceClientFactory {

    protected static final Logger LOG = Logger.getLogger(AmServiceClientFactory.class);

    /**
     * @return
     */
    public static ThriftClientFactory<AmService.Iface> newAmService() {
        return newAmService(null, null,
                            ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                            ThriftClientFactory.DEFAULT_MIN_IDLE,
                            ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     *
     * @param host
     * @param port
     * @return
     */
    public static ThriftClientFactory<AmService.Iface> newAmService(String host, Integer port) {
        return newAmService(host, port,
                                ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                ThriftClientFactory.DEFAULT_MIN_IDLE,
                                ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     *
     * @param host
     * @param port
     * @param maxPoolSize
     * @param minIdle
     * @param softMinEvictionIdleTimeMillis
     * @return
     */
    public static ThriftClientFactory<AmService.Iface> newAmService(String host, Integer port,
                                                                    int maxPoolSize,
                                                                    int minIdle,
                                                                    int softMinEvictionIdleTimeMillis) {
        return new ThriftClientFactory.Builder<AmService.Iface>()
                   .withHostPort(host, port)
                   .withPoolConfig(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis)
                   .withClientFactory(AmService.Client::new)
                   .build();
    }

    /**
     * @return
     */
    public static ThriftClientFactory<AsyncAmServiceRequest.Iface> newOneWayAsyncAmService() {
        return newOneWayAsyncAmService(null, null,
                                       ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                       ThriftClientFactory.DEFAULT_MIN_IDLE,
                                       ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     *
     * @param host
     * @param port
     * @return
     */
    public static ThriftClientFactory<AsyncAmServiceRequest.Iface> newOneWayAsyncAmService(String host, Integer port) {
        return newOneWayAsyncAmService(host, port,
                                       ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                       ThriftClientFactory.DEFAULT_MIN_IDLE,
                                       ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     *
     * @param host
     * @param port
     * @param maxPoolSize
     * @param minIdle
     * @param softMinEvictionIdleTimeMillis
     * @return
     */
    public static ThriftClientFactory<AsyncAmServiceRequest.Iface> newOneWayAsyncAmService(String host, Integer port,
                                                                    int maxPoolSize,
                                                                    int minIdle,
                                                                    int softMinEvictionIdleTimeMillis) {
        return new ThriftClientFactory.Builder<AsyncAmServiceRequest.Iface>()
                   .withHostPort(host, port)
                   .withPoolConfig(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis)
                   .withClientFactory(AsyncAmServiceRequest.Client::new)
                   .build();
    }

    /*
     * prevent instantiation
     */
    private AmServiceClientFactory() {}
}
