/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "pm_types.thrift"

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
 * bareAMState:  Tracks the state of the bare_am process, replaces fHasAm
 * javaAMState:  Tracks the state of the java_am process
 * dmState:      Tracks the state of the bare_am process, replaces fHasSm
 * smState:      Tracks the state of the bare_am process, replaces fHasDm
 *
 */

struct NodeInfo 
{
  1: required i64 uuid;
/* fields 2-5 are deprecated */
  2: required bool fHasAm;
  3: required bool fHasDm;
  4: required bool fHasOm;
  5: required bool fHasSm;

  6: required i32  bareAMPid = -1;
  7: required i32  javaAMPid = -1;
  8: required i32  dmPid = -1;
  9: required i32  smPid = -1;

 10: pm_types.pmServiceStateTypeId  bareAMState = pmServiceStateTypeId.SERVICE_NOT_PRESENT;
 11: pm_types.pmServiceStateTypeId  javaAMState = pmServiceStateTypeId.SERVICE_NOT_PRESENT;
 12: pm_types.pmServiceStateTypeId  dmState     = pmServiceStateTypeId.SERVICE_NOT_PRESENT;
 13: pm_types.pmServiceStateTypeId  smState     = pmServiceStateTypeId.SERVICE_NOT_PRESENT;
}
