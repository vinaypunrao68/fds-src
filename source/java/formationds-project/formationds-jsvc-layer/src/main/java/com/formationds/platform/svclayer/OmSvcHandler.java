/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.platform.svclayer;

import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.protocol.om.OMSvc;
import com.formationds.protocol.svc.CtrlNotifyDLTUpdate;
import com.formationds.protocol.svc.CtrlNotifyDMTUpdate;
import com.formationds.protocol.svc.GetAllVolumeDescriptors;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.thrift.TException;

public class OmSvcHandler extends PlatNetSvcHandlerBase<OMSvc.Iface> implements OMSvc.Iface {

    /**
     * @param thriftServiceName empty string if non-multiplexed server, otherwise a Thrift service name 
     */
    public OmSvcHandler( String host, int port, String thriftServiceName ) {
        this( PlatNetSvcHandlerBase.newClientFactory( OMSvc.Iface.class, OMSvc.Client::new, thriftServiceName, host, port ) );
    }

    protected OmSvcHandler( ThriftClientFactory<OMSvc.Iface> factory ) {
        super( factory );
    }

    @Override
    public void registerService( SvcInfo svcInfo ) throws TException {
        logger.trace( "registerService: {}", svcInfo );
        super.getOmNativePlatformClientFactory().getClient().registerService( svcInfo );
    }

    @Override
    public SvcInfo getSvcInfo( SvcUuid svcUuid ) throws TException {
        logger.trace( "getSvcInfo: {}", svcUuid );
        return super.getOmNativePlatformClientFactory().getClient().getSvcInfo( svcUuid );
    }

    @Override
    public CtrlNotifyDMTUpdate getDMT( long nullarg ) throws TException {
        logger.trace( "getDMT" );
        return super.getOmNativePlatformClientFactory().getClient().getDMT( nullarg );
    }

    @Override
    public CtrlNotifyDLTUpdate getDLT( long nullarg ) throws TException {
        logger.trace( "getDLT" );
        return super.getOmNativePlatformClientFactory().getClient().getDLT( nullarg );
    }
    
    @Override
    public GetAllVolumeDescriptors getAllVolumeDescriptors( long nullarg ) throws TException {
        logger.trace( "getAllVolumeDescriptors" );
    	return super.getOmNativePlatformClientFactory().getClient().getAllVolumeDescriptors( nullarg );
    }
}
