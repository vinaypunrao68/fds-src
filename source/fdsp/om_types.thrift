/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om.types

/**
 * Volume to be Tested
 */
struct FDSP_TestBucket {
  /** Volume name */
  1: string                 	bucket_name;
  /** Automatically attach if available */
  2: bool                   	attach_vol_reqd;
  /** Access Key */
  3: string                 	accessKeyId;
  /** Access Secret */
  4: string                 	secretAccessKey;
}

exception OmRegisterException {
}

exception SvcLookupException {
     1: string message;
}
