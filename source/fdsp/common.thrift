#ifndef __FDSCOMMON_H__
#define __FDSCOMMON_H__
namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.apis

enum ResourceState {
  Unknown,
  Loading, /* resource is loading or in the middle of creation */
  Created, /* resource has been created */
  Active,  /* resource activated - ready to use */
  Offline, /* resource is offline - will come back later */
  MarkedForDeletion, /* resource will be deleted soon. */
  Deleted, /* resource is gone now and will not come back*/
}

#endif