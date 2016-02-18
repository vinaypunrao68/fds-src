/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */
package com.formationds.util.thrift;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeSettings;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import javax.crypto.SecretKey;

/**
 * Proxy calls to the ConfigurationService interface in the OM to intercept
 * create/delete operations that must go through the Java OM for cluster management
 * and consensus.
 */
public class OMConfigurationServiceProxy implements InvocationHandler {

    private static final Logger logger = Logger.getLogger(OMConfigurationServiceProxy.class);

    /**
     *
     * @param secretKey
     * @param protocol
     * @param omHost
     * @param omHttpPort
     * @param omConfigPort
     *
     * @return the proxied OM Configuration API
     *
     * @throws TTransportException
     * @throws ConnectException
     */
    public static ConfigurationApi newOMConfigProxy( SecretKey secretKey,
                                                     String protocol,
                                                     String omHost,
                                                     int omHttpPort,
                                                     int omConfigPort ) {

        ThriftClientFactory<ConfigurationService.Iface> configService =
                ConfigServiceClientFactory.newConfigService( omHost, omConfigPort );

        logger.debug( "Attempting to connect to configuration API, on " +
                      omHost + ":" + omHttpPort + "." );

        return OMConfigurationServiceProxy.newOMConfigProxy( secretKey,
                                                             "http",
                                                             omHost,
                                                             omHttpPort,
                                                             configService.getClient( omHost,
                                                                                      omConfigPort ) );
    }

    /**
     * @param omConfigServiceClient
     * @param configApi
     * @return the proxy to the ConfigurationApi
     */
    private static ConfigurationApi newOMConfigProxy( SecretKey secretKey,
                                                      String protocol,
                                                      String omHost,
                                                      int omHttpPort,
                                                      ConfigurationService.Iface configApi) {

        return (ConfigurationApi) Proxy.newProxyInstance( OMConfigurationServiceProxy.class.getClassLoader(),
                                                          new Class<?>[] {ConfigurationApi.class},
                                                          new OMConfigurationServiceProxy( secretKey,
                                                                                           "http",
                                                                                           omHost,
                                                                                           omHttpPort,
                                                                                           configApi ) );
    }

    private final ConfigurationService.Iface  configurationService;

    private final SecretKey secretKey;
    private final String protocol;
    private final String omHost;
    private final int omHttpPort;

    private OMConfigServiceClient omConfigServiceClient;

    private OMConfigurationServiceProxy( SecretKey secretKey,
                                         String protocol,
                                         String omHost,
                                         int omHttpPort,
                                         ConfigurationService.Iface configurationApi ) {

        this.configurationService = configurationApi;
        this.secretKey = secretKey;
        this.protocol = protocol;
        this.omHost = omHost;
        this.omHttpPort = omHttpPort;
    }

    /**
     *
     * @return the currently configured OM rest client implementation, based on the {@link FdsFeatureToggles#OM_CONFIG_REST_CLIENT_PROXY_V08}
     * feature toggle
     */
    private OMConfigServiceClient getRestClient() {

        if (FdsFeatureToggles.OM_CONFIG_REST_CLIENT_PROXY_V08.isActive()) {
            if ( omConfigServiceClient == null ||
                 !( omConfigServiceClient instanceof OMConfigServiceRestClientv08Impl ) ) {

                omConfigServiceClient = new OMConfigServiceRestClientv08Impl( secretKey, protocol,
                                                                              omHost, omHttpPort );
            }
        } else {
            if ( omConfigServiceClient == null ||
                 !( omConfigServiceClient instanceof OMConfigServiceRestClientImpl ) ) {
                omConfigServiceClient = new OMConfigServiceRestClientImpl( secretKey, protocol,
                                                                           omHost, omHttpPort );
            }
        }
        return omConfigServiceClient;
    }

    @Override
    public Object invoke( Object proxy, Method method, Object[] args ) throws Throwable {

        long start = System.currentTimeMillis();
        try {
            if ( logger.isTraceEnabled() ) {
                logger.trace( String.format( "CONFIGPROXY::START:  %s(%s)",
                                             method.getName(),
                                             Arrays.toString( args ) ) );
            }
            switch ( method.getName() ) {

                // METHODS That should be redirected to the OM Rest API
                case "createVolume":
                    doCreateVolume( args );
                    return null;

                case "deleteVolume":
                    doDeleteVolume( args );
                    return null;

                // METHODS that are supported by OM Rest Client, but we are allowing
                // to go direct to the configuration api
                case "getVolumeId":
                case "statVolume":
                case "listVolumes":

                // METHODS that *should* probably be redirected but are not yet
                // supported by OM REST Client
                case "createTenant":
                case "createUser":
                case "assignUserToTenant":
                case "revokeUserFromTenant":
                case "updateUser":
                case "cloneVolume":
                case "createSnapshot":
                case "restoreClone":

                // Other methods can go direct to OM Thrift Server
                case "listTenants":
                case "allUsers":
                case "listUsersForTenant":
                case "configurationVersion":
                case "getVolumeName":
                case "registerStream":
                case "getStreamRegistrations":
                case "deregisterStream":
                case "createSnapshotPolicy":
                case "listSnapshotPolicies":
                case "deleteSnapshotPolicy":
                case "attachSnapshotPolicy":
                case "listSnapshotPoliciesForVolume":
                case "detachSnapshotPolicy":
                case "listVolumesForSnapshotPolicy":
                case "listSnapshots":
                default:
                    return method.invoke( configurationService, args );
            }
        } catch (Throwable t) {
            if (logger.isTraceEnabled()) {
                logger.trace(String.format("CONFIGPROXY::FAILED: %s(%s): %s",
                                           method.getName(),
                                           Arrays.toString(args),
                                           t.getMessage()));
            }

            if ( t instanceof InvocationTargetException ) {
                throw ((InvocationTargetException) t).getTargetException();
            } else {
                throw t;
            }
        } finally {
            if (logger.isTraceEnabled()) {
                logger.trace(String.format("CONFIGPROXY::COMPLETE: %s(%s) in %s ms",
                                           method.getName(),
                                           Arrays.toString(args),
                                           System.currentTimeMillis() - start));
            }
        }
    }

    private void doCreateVolume(Object... args) throws OMConfigException {
        if (logger.isTraceEnabled()) {
            logger.trace(String.format("CONFIGPROXY::HTTP::%-9s %s(%s)",
                                       "START:",
                                       "createVolume",
                                       Arrays.toString(args)));
        }

        try {
            AuthenticationToken token = AuthenticatedRequestContext.getToken();
            String domain = (String) args[0];
            String name = (String) args[1];
            VolumeSettings volumeSettings = (VolumeSettings) args[2];
            long tenantId = (Long) args[3];
            getRestClient().createVolume( token,
                                                domain,
                                                name,
                                                volumeSettings,
                                                tenantId );
        } finally {
            if (logger.isTraceEnabled()) {
                logger.trace(String.format("CONFIGPROXY::HTTP::%-9s %s(%s)",
                                           "COMPLETE:",
                                           "createVolume",
                                           Arrays.toString(args)));
            }
        }
    }

    private void doDeleteVolume(Object... args) throws OMConfigException {
        if (logger.isTraceEnabled()) {
            logger.trace(String.format("CONFIGPROXY::HTTP::%-9s %s(%s)",
                                       "START:",
                                       "deleteVolume",
                                       Arrays.toString(args)));
        }

        AuthenticationToken token = AuthenticatedRequestContext.getToken();
        String domain = (String)args[0];
        String name = (String)args[1];
        try {
            if ( omConfigServiceClient instanceof OMConfigServiceRestClientv08Impl ) {
                Long volumeId = configurationService.getVolumeId( name );
                ((OMConfigServiceRestClientv08Impl)getRestClient()).deleteVolume( token, volumeId );
            } else {
                getRestClient().deleteVolume(token, domain, name);
            }
        } catch (TException te) {
            throw new OMConfigException("Failed to load volume id for volume=" + name);
        } finally {
            if (logger.isTraceEnabled()) {
                logger.trace(String.format( "CONFIGPROXY::HTTP::%-9s %s(%s)",
                                            "COMPLETE:",
                                            "deleteVolume",
                                            Arrays.toString(args)));
            }
        }
    }
}
