/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CONCURRENCY_EVENTTRACKER_H_
#define SOURCE_INCLUDE_CONCURRENCY_EVENTTRACKER_H_
#include <shared/fds_types.h>
#include <thrift/concurrency/Monitor.h>
#include <map>
#include <string>
#include <atomic>

namespace fds {
    namespace concurrency{
        struct NumberedEventTracker {
            NumberedEventTracker();

            bool await(fds_uint64_t eventId, ulong timeout = 0);
            fds_uint64_t getUniqueEventId();
            // mark one task as complete
            void generate(fds_uint64_t eventId);

          protected:
            std::atomic<fds_uint64_t> eventCounter;
            std::map<fds_uint64_t, uint> mapEvents;
            apache::thrift::concurrency::Monitor monitor;
            apache::thrift::concurrency::Mutex mutex;
        };

        struct NamedEventTracker {
            NamedEventTracker();

            bool await(const std::string& event, ulong timeout = 0);

            // mark one task as complete
            void generate(const std::string& event);

          protected:
            std::map<std::string, uint> mapEvents;
            apache::thrift::concurrency::Monitor monitor;
            apache::thrift::concurrency::Mutex mutex;
        };
    }  // namespace concurrency //NOLINT
}  // namespace fds
#endif  // SOURCE_INCLUDE_CONCURRENCY_EVENTTRACKER_H_
