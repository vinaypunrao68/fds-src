package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_MsgHdrType;

public interface FdspResponseHandler<T> {
    void handle(FDSP_MsgHdrType messageType, T t);
}
