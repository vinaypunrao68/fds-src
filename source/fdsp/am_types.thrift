/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.am.types

/**
 * Message from OM to AM/SM/DM to help configure QoS control
 * For now using for OM setting total rate in AM based on current set of SMs
 * and their performance capabilities
 */
struct FDSP_QoSControlMsgType {
  /** total rate in FDS IOPs */
  1: i64    total_rate;
}

