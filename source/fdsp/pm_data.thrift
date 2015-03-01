/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef __platformtypes_h__
#define __platformtypes_h__

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace * FDS_ProtocolInterface

struct NodeInfo {
  1: i64 uuid,
  2: bool fHasAm,
  3: bool fHasDm,
  4: bool fHasOm,
  5: bool fHasSm,
}
#endif __platformtypes_h__