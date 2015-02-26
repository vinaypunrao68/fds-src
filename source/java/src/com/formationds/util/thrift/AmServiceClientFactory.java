/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */
package com.formationds.util.thrift;

import com.formationds.apis.XdiService;
import com.formationds.apis.AsyncXdiServiceRequest;
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
     * @return a ThriftClientFactory for the AmService.Iface with no default host or port
     */
    public static ThriftClientFactory<XdiService.Iface> newAmService() {
        return newAmService(null, null,
                            ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                            ThriftClientFactory.DEFAULT_MIN_IDLE,
                            ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     *
     * @param host the optional default thrift server host.
     * @param port the optional default thrift server port.  If a host is specified, then the port is required.
     *
     * @return a ThriftClientFactory for the AmService.Iface with the specified default host and port.
     */
    public static ThriftClientFactory<XdiService.Iface> newAmService(String host, Integer port) {
        return newAmService(host, port,
                                ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                ThriftClientFactory.DEFAULT_MIN_IDLE,
                                ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     * Create a new XdiService.Iface client factory.  The host and port are optional and if provided the
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
     * @return a ThriftClientFactory for the XdiService.Iface with the specified default host and port.
     */
    public static ThriftClientFactory<XdiService.Iface> newAmService(String host, Integer port,
                                                                    int maxPoolSize,
                                                                    int minIdle,
                                                                    int softMinEvictionIdleTimeMillis) {
        return new ThriftClientFactory.Builder<>(XdiService.Iface.class)
                   .withHostPort(host, port)
                   .withPoolConfig(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis)
                   .withClientFactory(XdiService.Client::new)
                   .build();
    }

    /**
     * @see #newOneWayAsyncAmService(String, Integer, int, int, int)
     */
    public static ThriftClientFactory<AsyncXdiServiceRequest.Iface> newOneWayAsyncAmService() {
        return newOneWayAsyncAmService(null, null,
                                       ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                       ThriftClientFactory.DEFAULT_MIN_IDLE,
                                       ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     * @see #newOneWayAsyncAmService(String, Integer, int, int, int)
     */
    public static ThriftClientFactory<AsyncXdiServiceRequest.Iface> newOneWayAsyncAmService(String host, Integer port) {
        return newOneWayAsyncAmService(host, port,
                                       ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                       ThriftClientFactory.DEFAULT_MIN_IDLE,
                                       ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     * Create a new AsyncXdiServiceRequest.Iface client factory.  The host and port are optional and if provided the
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
     * @return a ThriftClientFactory for the AsyncXdiServiceRequest.Iface with the specified default host and port.
     */
    public static ThriftClientFactory<AsyncXdiServiceRequest.Iface> newOneWayAsyncAmService(String host, Integer port,
                                                                    int maxPoolSize,
                                                                    int minIdle,
                                                                    int softMinEvictionIdleTimeMillis) {
        return new ThriftClientFactory.Builder<>(AsyncXdiServiceRequest.Iface.class)
                   .withHostPort(host, port)
                   .withPoolConfig(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis)
                   .withClientFactory(AsyncXdiServiceRequest.Client::new)
                   .build();
    }

    /*
     * prevent instantiation
     */
    private AmServiceClientFactory() {}
}
