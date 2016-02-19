package com.formationds.sc;

import com.formationds.apis.ConfigurationService;
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
        omBuilder = new Lazy<>(() -> makeClientFactory(OMSvc.Iface.class, OMSvc.Client::new));
        netSvcBuilder = new Lazy<>(() -> makeClientFactory(PlatNetSvc.Iface.class, PlatNetSvc.Client::new));
        configSvcBuilder = new Lazy<>(() -> makeClientFactory(ConfigurationService.Iface.class, ConfigurationService.Client::new));
        amSvcBuilder = new Lazy<>(() -> makeClientFactory(AMSvc.Iface.class, AMSvc.Client::new));
    }

    private <T> ThriftClientFactory<T> makeClientFactory(Class<T> klass, Function<TProtocol, T> clientFactory) {
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
