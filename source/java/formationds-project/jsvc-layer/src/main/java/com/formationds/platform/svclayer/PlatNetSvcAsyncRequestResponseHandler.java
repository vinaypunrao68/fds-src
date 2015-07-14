/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import com.formationds.protocol.svc.types.AsyncHdr;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.Marker;
import org.apache.logging.log4j.MarkerManager;
import org.apache.thrift.TBase;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;

public class PlatNetSvcAsyncRequestResponseHandler extends PlatNetSvcHandlerBase {

    static final Logger logger        = LogManager.getLogger();
    static final Marker SVC_REQUEST  = MarkerManager.getMarker( "SVC_REQUEST" );
    static final Marker SVC_RESPONSE = MarkerManager.getMarker( "SVC_RESPONSE" );

    private void logPayload( Marker marker, AsyncHdr asyncHdr, ByteBuffer payload ) {
        logger.trace( marker,
                      "Processing Header %s; Payload Size=%d", asyncHdr, payload.limit() );
        logger.trace( marker,
                      "Payload Data: 0x%s", HexDump.encodeHex( payload.array() ) );

        /* in order to deserialize the payload, we need to interrogate the Header
         * and extract the FDSPMsgTypeId.  Using the type id, we need to lookup the implementing Java class
         * in order to deserialize.  Of course, there is no direct link to the class -- we need to get the
         * typeid's enumerated name and hack off the "TypeId" and add the com.formationds.protocol.svc.types
         * package.
         */
        try {
            TBase o = SvcLayerSerializer.deserialize( asyncHdr.getMsg_type_id(), payload );
            logger.trace( marker,
                          "Deserialized Payload data: %s",
                          o );
        } catch ( Exception e ) {
            logger.warn( "Failed to deserialize payload data for " );
        }
    }

    @Override
    public void asyncReqt( AsyncHdr asyncHdr, ByteBuffer payload ) throws TException {
        logPayload( SVC_REQUEST, asyncHdr, payload );
        super.asyncReqt( asyncHdr, payload );
    }

    @Override
    public void asyncResp( AsyncHdr asyncHdr, ByteBuffer payload ) throws TException {
        logPayload( SVC_RESPONSE, asyncHdr, payload );
        super.asyncResp( asyncHdr, payload );
    }
}
