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
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.thrift.TException;

/**
 *
 */
public class ConfigServiceClientFactory extends ThriftClientFactory<ConfigurationService.Iface> {

    private class LegacyConfigServiceClientFactory extends ThriftClientFactory<FDSP_ConfigPathReq.Iface> {
        public LegacyConfigServiceClientFactory() { super(); }
        public LegacyConfigServiceClientFactory(int maxPoolSize, int minIdle, long softMinEvictionIdleTimeMillis) {
            super(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis);
        }
        @Override
        protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<FDSP_ConfigPathReq.Iface>> createClientPool() {
            ThriftClientConnectionFactory<FDSP_ConfigPathReq.Iface> legacyConfigFactory =
                new ThriftClientConnectionFactory<>(proto -> {
                    FDSP_Service.Client client = new FDSP_Service.Client(proto);
                    FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
                    try {
                        FDSP_SessionReqResp response = client.EstablishSession(msg);
                    } catch (TException e) {
                        LOG.error("Error establishing legacy FDSP_ConfigPathReq handshake", e);
                        throw new RuntimeException();
                    }
                    return new FDSP_ConfigPathReq.Client(proto);
                });
            return new GenericKeyedObjectPool<>(legacyConfigFactory, getConfig());
        }
    }

    private ThriftClientFactory<FDSP_ConfigPathReq.Iface> legacyConfigService = null;

    private String  defaultHost;
    private Integer defaultPort;

    /**
     * Create ConfigServiceClientFactory with default sizes
     */
    public ConfigServiceClientFactory() {
        super();
    }

    /**
     *
     * @param maxPoolSize
     * @param minIdle
     * @param softMinEvictionIdleTimeMillis
     */
    public ConfigServiceClientFactory(int maxPoolSize, int minIdle, long softMinEvictionIdleTimeMillis) {
        super(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis);
    }

    /**
     *
     * @param defaultHost
     * @param defaultPort
     */
    public ConfigServiceClientFactory(String defaultHost, int defaultPort) {
        this();
        this.defaultHost = defaultHost;
        this.defaultPort = defaultPort;
    }

    /**
     *
     * @param defaultHost
     * @param defaultPort
     * @param maxPoolSize
     * @param minIdle
     * @param softMinEvictionIdleTimeMillis
     */
    public ConfigServiceClientFactory(String defaultHost, int defaultPort, int maxPoolSize, int minIdle,
                                      long softMinEvictionIdleTimeMillis) {
        super(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis);
        this.defaultHost = defaultHost;
        this.defaultPort = defaultPort;
    }

    @Override
    protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<ConfigurationService.Iface>> createClientPool() {
        ThriftClientConnectionFactory<ConfigurationService.Iface> csFactory =
            new ThriftClientConnectionFactory<>(proto -> new ConfigurationService.Client(proto));
        return new GenericKeyedObjectPool<>(csFactory, super.getConfig());
    }

    /**
     * Connect using the default host and port.
     *
     * @return the client
     *
     * @throws NullPointerException if default host or port is null.
     */
    public ConfigurationService.Iface getClient() {
        return super.getClient(defaultHost, defaultPort);
    }

    /**
     * @return a handle to the legacy config service
     */
    public ThriftClientFactory<Iface> getLegacyConfigService() {
        if (legacyConfigService == null) {
            legacyConfigService = new LegacyConfigServiceClientFactory(getConfig().getMaxTotal(),
                                                                       getConfig().getMinIdlePerKey(),
                                                                       getConfig().getSoftMinEvictableIdleTimeMillis());
        }
        return legacyConfigService;
    }

}
