/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

struct FDSP_TestBucket {
  1: string                 	bucket_name;
  2: common.FDSP_VolumeDescType	vol_info; /* Bucket properties and attributes */
  3: bool                   	attach_vol_reqd; /* Should OMgr send out an attach volume if the bucket is accessible, etc */
  4: string                 	accessKeyId;
  5: string                 	secretAccessKey;
}
