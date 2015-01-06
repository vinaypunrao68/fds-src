/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "disk_object.h"

namespace fds
{
    DiskObject::DiskObject() : Resource(ResourceUUID()), dsk_type_link(this)
    {
    }

    DiskObject::~DiskObject()
    {
    }

}  // namespace fds
