/* Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <map>
#include <unordered_map>
#include <fds_assert.h>
#include <platform/flags_map.h>

namespace fds
{

    /* Definition of common flags */
    DBG(DEFINE_FLAG(common_drop_async_resp));
    DBG(DEFINE_FLAG(common_enter_gdb));

    /**
     * @brief Common flags registration
     */
    void FlagsMap::registerCommonFlags()
    {
        /* Common flags registration */
        DBG(REGISTER_FLAG((*this), common_drop_async_resp));
        DBG(REGISTER_FLAG((*this), common_enter_gdb));
    }

    FlagsMap::FlagsMap()
    {
    }

    FlagsMap::~FlagsMap()
    {
    }

    void FlagsMap::registerFlag(const std::string &id, int64_t *flag)
    {
        auto    itr = flags_.find(id);

        if (itr != flags_.end())
        {
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
    bool FlagsMap::setFlag(const std::string &id, int64_t value)
    {
        auto    itr = flags_.find(id);

        if (itr == flags_.end())
        {
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
    bool FlagsMap::getFlag(const std::string &id, int64_t &value)
    {
        auto    itr = flags_.find(id);

        if (itr == flags_.end())
        {
            return false;
        }

        value = *(itr->second);

        return true;
    }

    /**
     * @brief return flags map
     *
     * @return
     */
    std::map<std::string, int64_t> FlagsMap::toMap()
    {
        std::map<std::string, int64_t>    ret;

        for (auto kv : flags_)
        {
            ret[kv.first] = *(kv.second);
        }

        return ret;
    }
}  // namespace fds
