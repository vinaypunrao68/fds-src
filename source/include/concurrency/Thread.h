/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_CONCURRENCY_THREAD_H_
#define SOURCE_UTIL_CONCURRENCY_THREAD_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>

typedef boost::condition   fds_condition;
typedef boost::shared_ptr<fds_condition> FdsConditionPtr;

#endif  // SOURCE_UTIL_CONCURRENCY_THREAD_H_
