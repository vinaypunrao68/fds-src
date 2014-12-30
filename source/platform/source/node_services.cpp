/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <stdint.h>

#include <platform/node_services.h>

namespace fds
{
    uint32_t NodeServices::write(serialize::Serializer*  s) const
    {
        uint32_t    b = 0;
        b += s->writeI64(sm.uuid_get_val());
        b += s->writeI64(dm.uuid_get_val());
        b += s->writeI64(am.uuid_get_val());
        return b;
    }

    uint32_t NodeServices::read(serialize::Deserializer* s)
    {
        uint32_t        b = 0;
        fds_uint64_t    uuid;
        b += s->readI64(uuid);
        sm = uuid;
        b += s->readI64(uuid);
        dm = uuid;
        b += s->readI64(uuid);
        am = uuid;

        return b;
    }
}  // namespace fds
