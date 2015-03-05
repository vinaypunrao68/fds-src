/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

/**
 * Volume to be Tested
 */
struct FDSP_TestBucket {
  /** Volume name */
  1: string                 	bucket_name;
  /** Volume specification */
  2: common.FDSP_VolumeDescType	vol_info;
  /** Automatically attach if available */
  3: bool                   	attach_vol_reqd;
  /** Access Key */
  4: string                 	accessKeyId;
  /** Access Secret */
  5: string                 	secretAccessKey;
}
