/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

/* This file is only for adding test thrift services. It's kept in this directory because
 * the build dependencies are easier to manage, if we keep in this directory
 */

include "fds_service.thrift"

namespace cpp FDS_ProtocolInterface

service TestSMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void putObject(1: fds_service.AsyncHdr asyncHdr, 2: fds_service.PutObjectMsg payload);
}

service TestDMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void updateCatalog(1: fds_service.AsyncHdr asyncHdr, 2: fds_service.UpdateCatalogOnceMsg payload);
}

service TestAMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void putObjectRsp(1: fds_service.AsyncHdr asyncHdr, 2: fds_service.PutObjectRspMsg payload);
    oneway void updateCatalogRsp(1: fds_service.AsyncHdr asyncHdr, 2: fds_service.UpdateCatalogOnceRspMsg payload);
}
