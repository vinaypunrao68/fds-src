/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_PLAT_SERIALIZE_H_
#define SOURCE_INCLUDE_PLATFORM_PLAT_SERIALIZE_H_

#include <vector>
#include <string>
#include <ostream>
#include <serialize.h>
#include <fds_resource.h>

namespace fds {

struct NodeServices : serialize::Serializable
{
    ResourceUUID sm,dm,am;

    inline void reset() {
        sm.uuid_set_val(0);
        dm.uuid_set_val(0);
        am.uuid_set_val(0);
    }

    uint32_t virtual write(serialize::Serializer*  s) const;
    uint32_t virtual read(serialize::Deserializer* s);
    friend std::ostream& operator<< (std::ostream &os, const NodeServices& node);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_PLAT_SERIALIZE_H_
