/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef __platformtypes_h__
#define __platformtypes_h__

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm.types

struct NodeInfo {
  1: required i64 uuid,
  2: required bool fHasAm,
  3: required bool fHasDm,
  4: required bool fHasOm,
  5: required bool fHasSm,
  6: required i32  bareAMPid
  7: required i32  javaAMPid
  8: required i32  dmPid
  9: required i32  smPid
}

#endif __platformtypes_h__
