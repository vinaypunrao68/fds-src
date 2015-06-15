/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef __platformtypes_h__
#define __platformtypes_h__

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm.types

/**
 * Information stored locally to a node.
 *
 * uuid:       The FUUID (formation UUID) of the Node
 * fHasAm:     Flag that indicates if this node is typically expected to have an AM operating
 * fHasDm:     Flag that indicates if this node is typically expected to have a DM operating
 * fHasOm:     Flag that indicates if this node is typically expected to have an OM operating (PRESENTLY NOT USED)
 * fHasSm:     Flag that indicates if this node is typically expected to have a SM operating
 * bareAMPid:  Pid of the running or most recently running bare_am process
 * javaAMPid:  Pid of the running or most recently running java process that loaded the am.main class
 * dmPid:      Pid of the running or most recently running DataMgr process
 * smPid:      Pid of the running or most recently running StorMgr process
 *
 */

struct NodeInfo {
  1: required i64 uuid,
  2: required bool fHasAm,
  3: required bool fHasDm,
  4: required bool fHasOm,
  5: required bool fHasSm,
  6: i32  bareAMPid = -1,
  7: i32  javaAMPid = -1,
  8: i32  dmPid = -1,
  9: i32  smPid = -1,
}

#endif __platformtypes_h__
