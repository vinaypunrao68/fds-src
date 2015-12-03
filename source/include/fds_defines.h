/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_DEFINES_H_
#define SOURCE_INCLUDE_FDS_DEFINES_H_

/**
 * System wide defines and macros
 */

#define NUM_BITS_FOR_TOKENS 16
#define MAX_TOKENS (1 << NUM_BITS_FOR_TOKENS)
#define MAX_REPL_FACTOR 4
#define KB 1024
#define MB (KB*1024)
#define GB (MB*1024)
#define NANOS_IN_SECOND 1000000000
#define NANOS_IN_MINUTE (NANOS_IN_SECOND * 60)
#define NANOS_IN_HOUR   (NANOS_IN_MINUTE * 60)

#define ENUMCASEOS(x, os) case x : os << #x ; break;

#define SHARED_DYN_CAST(T, ptr) boost::dynamic_pointer_cast<T>(ptr)
// we can change to static_cast later
#define TO_DERIVED(T,ptr) dynamic_cast<T*>(ptr) //NOLINT

#define TYPE_SHAREDPTR(T) typedef boost::shared_ptr<T> ptr
#define SHPTR boost::shared_ptr

#ifndef DONTLOGLINE
#define _ATLINE_ << "[" __FILE__ << ":" << std::dec << __LINE__ << ":" << __FUNCTION__ << "] - "
#else
#define _ATLINE_
#endif

#define __CLASS_NAME__ fds::classNameFromPrettyFunc(__PRETTY_FUNCTION__)
#define LOGCONSOLE std::cerr _ATLINE_
#define PH_ARG1 std::placeholders::_1
#define PH_ARG2 std::placeholders::_2
#define PH_ARG3 std::placeholders::_3

#endif  // SOURCE_INCLUDE_FDS_DEFINES_H_
