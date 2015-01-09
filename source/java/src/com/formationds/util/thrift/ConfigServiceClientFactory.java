/**
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
     * @return
     */
    public static ThriftClientFactory<FDSP_ConfigPathReq.Iface> newLegacyConfigService() {
        return newLegacyConfigService(null, null,
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
    public static ThriftClientFactory<FDSP_ConfigPathReq.Iface> newLegacyConfigService(String host, Integer port) {
        return newLegacyConfigService(host, port,
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
    public static ThriftClientFactory<FDSP_ConfigPathReq.Iface> newLegacyConfigService(String host, Integer port,
                                                                                        int maxPoolSize,
                                                                                        int minIdle,
                                                                                        int softMinEvictionIdleTimeMillis) {
        return new ThriftClientFactory.Builder<FDSP_ConfigPathReq.Iface>()
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
     * @return
     */
    public static ThriftClientFactory<ConfigurationService.Iface> newConfigService() {
        return newConfigService(null, null,
                                ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                ThriftClientFactory.DEFAULT_MIN_IDLE,
                                ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS);
    }

    /**
     * @param host
     * @param port
     * @return
     */
    public static ThriftClientFactory<ConfigurationService.Iface> newConfigService(String host, Integer port) {
        return newConfigService(host, port,
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
    public static ThriftClientFactory<ConfigurationService.Iface> newConfigService(String host, Integer port,
                                                                                    int maxPoolSize,
                                                                                    int minIdle,
                                                                                    int softMinEvictionIdleTimeMillis) {
        return new ThriftClientFactory.Builder<ConfigurationService.Iface>()
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
