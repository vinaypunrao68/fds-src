/* Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <unordered_map>
#include <fds_assert.h>
#include <platform/fds_flags.h>

namespace fds {


FlagsMap::FlagsMap()
{
}

FlagsMap::~FlagsMap()
{
}

void FlagsMap::registerFlag(const std::string &id, uint64_t *flag)
{
    auto itr = flags_.find(id);
    if (itr != flags_.end()) {
        fds_assert(!"Registering already registered flag");
        return;
    }
    flags_[id] = flag;
}

/**
* @brief Sets the flag
*
* @param id flag id
* @param value value to set
*
* @return true if flag is set
*/
bool FlagsMap::setFlag(const std::string &id, uint64_t value)
{
    auto itr = flags_.find(id);
    if (itr == flags_.end()) {
        return false;
    }
    *(itr->second) = value;
    return true;
}

/**
* @brief returns flag value
*
* @param id flag id
* @param value return value of the flag
*
* @return true if flag with id is found
*/
bool FlagsMap::getFlag(const std::string &id, uint64_t &value)
{
    auto itr = flags_.find(id);
    if (itr == flags_.end()) {
        return false;
    }
    value = *(itr->second);
    return true;
}

/**
* @brief Common flags registration
*/
void FlagsMap::registerCommonFlags()
{
    /* Common flags registration */
    DBG(REGISTER_FLAG((*this), common_drop_async_resp));
}

}  // namespace fds
