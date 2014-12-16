/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_OBJECT_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_OBJECT_H_

#include <fds_resource.h>

namespace fds
{
    class DiskObjIter;
    class DiskInventory;

    /**
     * Generic disk obj shared by PM and other daemons such as OM.
     */

    class DiskObject : public Resource
    {
        public:
            typedef boost::intrusive_ptr<DiskObject> pointer;
            typedef boost::intrusive_ptr<const DiskObject> const_ptr;

        protected:
            friend class DiskInventory;

            fds_uint64_t    dsk_cap_gb;
            dev_t           dsk_my_devno;
            ChainLink       dsk_type_link;        /**< link to hdd/ssd list...      */

        public:
            DiskObject();
            virtual ~DiskObject();

            static inline DiskObject::pointer dsk_cast_ptr(Resource::pointer ptr)
            {
                return static_cast<DiskObject *>(get_pointer(ptr));
            }
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_OBJECT_H_
