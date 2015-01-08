/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import com.formationds.apis.ConfigurationService;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;

/**
 *
 */
public class ConfigServiceClientFactory extends ThriftClientFactory<ConfigurationService.Iface> {

    public ConfigServiceClientFactory(int maxPoolSize, int minIdle, long softMinEvictionIdleTimeMillis) {
        super(maxPoolSize, minIdle, softMinEvictionIdleTimeMillis);
    }

    @Override
    protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<ConfigurationService.Iface>> createClientPool() {
        ThriftClientConnectionFactory<ConfigurationService.Iface> csFactory =
            new ThriftClientConnectionFactory<>(proto -> new ConfigurationService.Client(proto));
        return new GenericKeyedObjectPool<>(csFactory, super.getConfig());
    }
}
