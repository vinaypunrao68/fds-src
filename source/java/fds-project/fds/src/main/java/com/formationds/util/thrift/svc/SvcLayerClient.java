/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.util.thrift.svc;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.commons.model.AccessManagerService;
import com.formationds.commons.model.DataManagerService;
import com.formationds.commons.model.Domain;
import com.formationds.commons.model.Node;
import com.formationds.commons.model.PlatformManagerService;
import com.formationds.commons.model.Service;
import com.formationds.commons.model.StorageManagerService;
import com.formationds.commons.model.type.NodeState;
import com.formationds.commons.model.type.ServiceStatus;
import com.formationds.commons.model.type.ServiceType;
import com.formationds.protocol.commonConstants;
import com.formationds.protocol.svc.PlatNetSvc;
import com.formationds.protocol.svc.types.DomainNodes;
import com.formationds.protocol.svc.types.NodeSvcInfo;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.util.thrift.ThriftClientFactory;
import com.google.common.net.HostAndPort;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author ptinius
 */
public class SvcLayerClient
    implements SvcLayerClientIface {

    private static final Logger logger =
        LoggerFactory.getLogger( SvcLayerClient.class );

    private static final Integer DEF_PM_PORT = 7000;

    private ThriftClientFactory<PlatNetSvc.Iface> netsvc;

    private final HostAndPort host;

    public SvcLayerClient( ) {

        this( HostAndPort.fromParts( "localhost", DEF_PM_PORT ) );

    }

    public SvcLayerClient( final HostAndPort host ) {

        this.host = host;

        if ( FdsFeatureToggles.SUBSCRIPTIONS.isActive() ) {
            netsvc =
                new ThriftClientFactory.Builder<>( PlatNetSvc.Iface.class )
                    .withThriftServiceName( commonConstants.PLATNET_SERVICE_NAME )
                    .withClientFactory( PlatNetSvc.Client::new )
                    .withHostPort( host.getHostText(),
                                   host.getPortOrDefault(
                                       DEF_PM_PORT ) )
                    .build();

        } else {
            netsvc =
                new ThriftClientFactory.Builder<>( PlatNetSvc.Iface.class )
                    .withClientFactory( PlatNetSvc.Client::new )
                    .withHostPort( host.getHostText(),
                                   host.getPortOrDefault(
                                       DEF_PM_PORT ) )
                    .build();
        }
        logger.debug( "connecting to service on {}", host );
    }

    public HostAndPort getHost( ) {
        return host;
    }

    @Override
    public boolean equals( final Object o ) {

        if( this == o ) {

            return true;
        }

        if( !( o instanceof SvcLayerClient ) ) {

            return false;
        }

        final SvcLayerClient that = ( SvcLayerClient ) o;

        return host.equals( that.host ) &&
            !( netsvc != null
                ? !netsvc.equals( that.netsvc )
                : that.netsvc != null );

    }

    @Override
    public int hashCode( ) {

        return 31 * ( netsvc != null ? netsvc.hashCode() : 0 )
            + ( host.hashCode() );
    }

    @Override
    public Map<Domain, List<Node>> getDomainNodes( final DomainNodes domainNodes )
        throws SvcLayerException {

        final Map<Domain, List<Node>> ret = new HashMap<>();

        try {

            final DomainNodes dnodes =
                netsvc.getClient()
                      .getDomainNodes( domainNodes );

            final Domain domain = Domain.uuid( dnodes.getDom_id()
                                                     .getDomain_id()
                                                     .getSvc_uuid() )
                                        .domain( dnodes.getDom_id()
                                                       .getDomain_name() )
                                        .build();
            ret.put( domain, new ArrayList<>( ) );

            for( final NodeSvcInfo nodeInfo : dnodes.getDom_nodes() ) {

		logger.trace( "NODE UUID::" + 
		    	      nodeInfo.getNode_base_uuid()
				      .getSvc_uuid() );
		nodeInfo.getNode_svc_list().stream().forEach((svcInfo) -> {
			logger.trace( "SVC::" + svcInfo.toString() ); });
		
                final Node node =
                    Node.uuid( String.valueOf( nodeInfo.getNode_base_uuid()
                                                       .getSvc_uuid() ) )
                        .ipV6address( "::1" ) // default for now to localhost
                        .ipV4address( nodeInfo.getNode_addr() )
                        .name( nodeInfo.getNode_auto_name() )
                        .state(
                            NodeState.byFdsDefined( nodeInfo.getNode_state()
                                                            .name() )
                                     .get() )
                        .build();
                for( final SvcInfo svcInfo : nodeInfo.getNode_svc_list() ) {

                    Service service = null;
                    ServiceType type = null;
                    switch( svcInfo.getSvc_type() ) {
                        case FDSP_ACCESS_MGR:
                            service = new AccessManagerService();
                            type = ServiceType.AM;
                            break;
                        case FDSP_DATA_MGR:
                            service = new DataManagerService();
                            type = ServiceType.DM;
                            break;
                        case FDSP_PLATFORM:
                            service = new PlatformManagerService();
                            type = ServiceType.PM;
                            break;
                        case FDSP_STOR_MGR:
                            service = new StorageManagerService();
                            type = ServiceType.SM;
                            break;
                        // TODO should we include the OM or other services?
                    }

                    if( ( service != null ) && ( type != null ) ) {

                        final Optional<ServiceStatus> status =
                            ServiceStatus.fromThriftServiceStatus(
                                svcInfo.getSvc_status() );

                        service.setUuid( svcInfo.getSvc_id()
                                                .getSvc_uuid()
                                                .getSvc_uuid() );
                        if( status.isPresent() ) {

                            service.setStatus( status.get() );

                        }
                        service.setPort( svcInfo.getSvc_port() );
                        service.setAutoName( type.name() );
                        service.setName( svcInfo.getSvc_auto_name() );

                        node.addService( service );
                    }
                }

                logger.trace( node.toString() );
                ret.get( domain ).add( node );
            }

        } catch( TException e ) {

            throw new SvcLayerException( "Failed getting domain nodes (%s)",
                                          new Object[] { getHost().toString() } );
        }

        ret.forEach( (k,v) -> logger.trace( "KEY::{} VALUES::{}", k, v ) );
        return ret;
    }
}
