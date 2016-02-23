package com.formationds.sc;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;

import com.formationds.apis.ConfigurationService;
import com.formationds.protocol.commonConstants;
import com.formationds.protocol.am.AMSvc;
import com.formationds.protocol.om.OMSvc;
import com.formationds.protocol.svc.PlatNetSvc;
import com.formationds.util.Lazy;
import com.formationds.util.thrift.ThriftClientFactory;
import com.google.common.net.HostAndPort;
import org.apache.thrift.protocol.TProtocol;
import java.util.function.Function;

public class ClientFactory {
    private static final int CONN_MIN = 1;
    private static final int CONN_MAX = 1000;
    private static final int EVICTION_MS = 30000;

    private final Lazy<ThriftClientFactory<OMSvc.Iface>> omBuilder;
    private final Lazy<ThriftClientFactory<PlatNetSvc.Iface>> netSvcBuilder;
    private final Lazy<ThriftClientFactory<ConfigurationService.Iface>> configSvcBuilder;
    private final Lazy<ThriftClientFactory<AMSvc.Iface>> amSvcBuilder;

    public ClientFactory() {
        omBuilder = new Lazy<>(() -> makeClientFactory(OMSvc.Iface.class,
            commonConstants.OM_SERVICE_NAME,
            OMSvc.Client::new));
        netSvcBuilder = new Lazy<>(() -> makeClientFactory(PlatNetSvc.Iface.class,
            commonConstants.PLATNET_SERVICE_NAME,
            PlatNetSvc.Client::new));
        configSvcBuilder = new Lazy<>(() -> makeClientFactory(ConfigurationService.Iface.class,
            commonConstants.CONFIGURATION_SERVICE_NAME,
            ConfigurationService.Client::new));
        amSvcBuilder = new Lazy<>(() -> makeClientFactory(AMSvc.Iface.class,
            commonConstants.AM_SERVICE_NAME,
            AMSvc.Client::new));
    }

    private <T> ThriftClientFactory<T> makeClientFactory(Class<T> klass, String thriftServiceName, Function<TProtocol, T> clientFactory) {
        /**
         * FEATURE TOGGLE: enable multiplexed services
         * Tue Feb 23 15:04:17 MST 2016
        */
        if ( FdsFeatureToggles.THRIFT_MULTIPLEXED_SERVICES.isActive() ) {
            // Always assume multiplexed server, use a ThriftServiceName
            return new ThriftClientFactory.Builder<T>(klass)
                    .withClientFactory(clientFactory)
                    .withThriftServiceName(thriftServiceName)
                    .withPoolConfig(CONN_MIN, CONN_MAX, EVICTION_MS)
                    .build();
        }
        // TBinaryProtocol, no ThriftServiceName needed
        return new ThriftClientFactory.Builder<T>(klass)
                .withClientFactory(clientFactory)
                .withPoolConfig(CONN_MIN, CONN_MAX, EVICTION_MS)
                .build();
    }

    OMSvc.Iface omClient(HostAndPort omAddr) {
        return omBuilder.getInstance().getClient(omAddr.getHostText(), omAddr.getPort());
    }

    PlatNetSvc.Iface platNetSvcClient(HostAndPort addr) {
        return netSvcBuilder.getInstance().getClient(addr.getHostText(), addr.getPort());
    }

    ConfigurationService.Iface configServiceClient(HostAndPort addr) {
        return configSvcBuilder.getInstance().getClient(addr.getHostText(), addr.getPort());
    }

    AMSvc.Iface amServiceClient(HostAndPort addr) {
        return amSvcBuilder.getInstance().getClient(addr.getHostText(), addr.getPort());
    }
}
