/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef __FDSCOMMON_H__
#define __FDSCOMMON_H__

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

enum ResourceState {
  Unknown,
  Loading, /* resource is loading or in the middle of creation */
  Created, /* resource has been created */
  Active,  /* resource activated - ready to use */
  Offline, /* resource is offline - will come back later */
  MarkedForDeletion, /* resource will be deleted soon. */
  Deleted, /* resource is gone now and will not come back*/
}

enum BlobListOrder {
    UNSPECIFIED = 0,
    LEXICOGRAPHIC,
    BLOBSIZE
}

/* Can be consolidated when apis and fdsp merge or whatever */
struct BlobDescriptor {
     1: required string name,
     2: required i64 byteCount,
     3: required map<string, string> metadata
}

/* A detailed list of blob stats. */
typedef list<BlobDescriptor> BlobDescriptorListType

#endif
