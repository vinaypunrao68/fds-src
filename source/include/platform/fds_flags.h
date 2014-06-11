/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_FLAGS_H_
#define SOURCE_INCLUDE_FDS_FLAGS_H_

#include <string>
#include <unordered_map>
#include <fds_assert.h>

/* Use this macro for declaring a flag */
#define DECLARE_FLAG(id)                    uint64_t id
/* Use this macro for registering a flag */
#define REGISTER_FLAG(map, id)  \
    id = 0; \
    map.registerFlag(#id, &id)

namespace fds {


/*--------------------------------------------------------*/


class FlagsMap {
 public:
    FlagsMap();
    ~FlagsMap();

    void registerFlag(const std::string &id, uint64_t *flag);
    bool setFlag(const std::string &id, uint64_t value);
    bool getFlag(const std::string &id, uint64_t &value);
    
    void registerCommonFlags();

 protected:
    std::unordered_map<std::string, uint64_t*> flags_;

    /* Declaration of flags that are common to all services */
    /* Drop async responses */
    DBG(DECLARE_FLAG(common_drop_async_resp));
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_FLAGS_H_
