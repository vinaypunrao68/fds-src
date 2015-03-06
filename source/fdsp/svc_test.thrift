/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "fds_service.thrift"
include "dm_api.thrift"
include "sm_api.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.am

/* ------------------------------------------------------------
   Other specified services
   ------------------------------------------------------------*/

/**
 * Some service layer test thingy.
 */
service TestAMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void putObjectRsp(1: fds_service.AsyncHdr asyncHdr, 2: sm_api.PutObjectRspMsg payload);
    oneway void updateCatalogRsp(1: fds_service.AsyncHdr asyncHdr, 2: dm_api.UpdateCatalogOnceRspMsg payload);
}

/**
 * Some service layer test thingy.
 */
service TestDMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void updateCatalog(1: fds_service.AsyncHdr asyncHdr, 2: dm_api.UpdateCatalogOnceMsg payload);
}

/**
 * Some service layer test thingy.
 */
service TestSMSvc {
  /**   */
    i32 associate(1: string myip, 2: i32 port);
  /**   */
    oneway void putObject(1: fds_service.AsyncHdr asyncHdr, 2: sm_api.PutObjectMsg payload);
}
