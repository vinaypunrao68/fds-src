/*
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_ConfigPathReq.Iface;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Service;
import FDS_ProtocolInterface.FDSP_SessionReqResp;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ConfigurationService.Client;
import com.formationds.util.Configuration;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
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
     * @see #newLegacyConfigService(String, Integer, int, int, int)
     */
    public static ThriftClientFactory<FDSP_ConfigPathReq.Iface> newLegacyConfigService() {
        return newLegacyConfigService(null, null,
                                      ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                      ThriftClientFactory.DEFAULT_MIN_IDLE,
                                      ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     * @see #newLegacyConfigService(String, Integer, int, int, int)
     */
    public static ThriftClientFactory<FDSP_ConfigPathReq.Iface> newLegacyConfigService(String host, Integer port) {
        return newLegacyConfigService(host, port,
                                      ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                      ThriftClientFactory.DEFAULT_MIN_IDLE,
                                      ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     *
     * Create a new FDSP_ConfigPathReq.Iface client factory.  The host and port are optional and if provided the
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
     * @return a ThriftClientFactory for the FDSP_ConfigPathReq.Iface with the specified default host and port.
     */
    public static ThriftClientFactory<FDSP_ConfigPathReq.Iface> newLegacyConfigService(String host, Integer port,
                                                                                        int maxPoolSize,
                                                                                        int minIdle,
                                                                                        int softMinEvictionIdleTimeMillis) {
        return new ThriftClientFactory.Builder<>(FDSP_ConfigPathReq.Iface.class)
                   .withHostPort(host, port)
                   .withPoolConfig(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis)
                   .withClientFactory(proto -> {
                                          FDSP_Service.Client client = new FDSP_Service.Client(proto);
                                          FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
                                          try {
                                              FDSP_SessionReqResp response = client.EstablishSession(msg);
                                          } catch (TException e) {
                                              LOG.error("Error establishing legacy FDSP_ConfigPathReq handshake", e);
                                              throw new RuntimeException();
                                          }
                                          return new FDSP_ConfigPathReq.Client(proto);
                                      })
                                      .build();
    }

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
