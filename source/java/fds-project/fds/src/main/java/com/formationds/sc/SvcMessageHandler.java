package com.formationds.sc;

import com.formationds.protocol.svc.types.FDSPMsgTypeId;

public @interface SvcMessageHandler {
    FDSPMsgTypeId messageType();
}
