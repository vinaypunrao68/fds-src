/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_FLAGS_MAP_H_
#define SOURCE_INCLUDE_PLATFORM_FLAGS_MAP_H_

#include <string>
#include <map>
#include <unordered_map>

#include "fds_assert.h"

/* Use this macro for declaring a flag */
#define DECLARE_FLAG(id)                    extern int64_t id

#define DEFINE_FLAG(id)                     int64_t id

/* Use this macro for registering a flag */
#define REGISTER_FLAG(map, id)  \
    id = 0; \
    map.registerFlag(# id, &id)

/* Return if condition is true */
#define FLAG_CHECK_RETURN_VOID(cond) \
    do { \
        if ((cond)) { \
            return; \
        } \
    } while (false)

namespace fds
{
    /* Declaration of flags that are common to all services */
    /* Drop async responses */
    DBG(DECLARE_FLAG(common_drop_async_resp));

    /* Enter gdb */
    DBG(DECLARE_FLAG(common_enter_gdb));

    class FlagsMap
    {
        public:
            FlagsMap();
            ~FlagsMap();

            void registerFlag(const std::string &id, int64_t *flag);
            bool setFlag(const std::string &id, int64_t value);
            bool getFlag(const std::string &id, int64_t &value);

            void registerCommonFlags();

            std::map<std::string, int64_t> toMap();

        protected:
            std::unordered_map<std::string, int64_t*>    flags_;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_FLAGS_MAP_H_
