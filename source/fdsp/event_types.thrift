
/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om.event.types

enum EventType {
  USER_ACTIVITY = 0,
  SYSTEM_EVENT,
  FIREBREAK_EVENT;	
}

enum EventState {
  SOFT = 0,
  HARD,
  RECOVERED;
}

enum EventSeverity {
  INFO = 0;
  CONFIG,
  WARNING,
  ERROR,
  CRITICAL,
  FATAL,
  UNKNOWN;
}

enum EventCategory {
  FIREBREAK = 0,
  VOLUMES,
  STORAGE,
  SYSTEM,
  PERFOMRANCE;
}

/**
 * @param type the event type
 * @param category the event category
 * @param severity the event severity
 * @param defaultMessage the default message
 * @param argumentNames the message argument names
 * @param key the message key
 */
struct EventDescriptor {
  1: required EventType type;
  2: required EventCategory category;
  3: required EventSeverity severity;
  4: required string defaultMessage;
  5: required list<string> argumentNames;
  6: required string key;
}
