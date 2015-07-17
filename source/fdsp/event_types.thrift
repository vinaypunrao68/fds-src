
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
  EVENT_STATE_SOFT = 0,
  EVENT_STATE_HARD,
  EVENT_STATE_RECOVERED;
}

enum EventSeverity {
  EVENT_SEVERITY_INFO = 0;
  EVENT_SEVERITY_CONFIG,
  EVENT_SEVERITY_WARNING,
  EVENT_SEVERITY_ERROR,
  EVENT_SEVERITY_CRITICAL,
  EVENT_SEVERITY_FATAL,
  EVENT_SEVERITY_UNKNOWN;
}

enum EventCategory {
  EVENT_CATEGORY_FIREBREAK = 0,
  EVENT_CATEGORY_VOLUMES,
  EVENT_CATEGORY_STORAGE,
  EVENT_CATEGORY_SYSTEM,
  EVENT_CATEGORY_PERFORMANCE;
}

union MessageArgs {
 1: i32 int32;
 2: i64 int64;
 3: double dbl;
 4: string str;
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

struct Event {
  1: required EventType type;
  2: required EventCategory category;
  3: required EventSeverity severity;
  4: required EventState state;
  5: required i64 initialTimestamp;
  6: required i64 modifiedTimestamp;
  7: required string messageKey;
  8: required list<MessageArgs> messageArgs;
  9: required string messageFormat;
 10: required string defaultMessage;
}
