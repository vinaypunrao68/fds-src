/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Volume metadata class. Describes the per-volume metada
 * that is maintained by a data manager.
 */

#ifndef SOURCE_DATA_MANAGER_VOLUMEMETA_H_
#define SOURCE_DATA_MANAGER_VOLUMEMETA_H_

#include <string>

#include "include/fds_types.h"
#include "include/fds_err.h"
#include "lib/VolumeCatalog.h"

namespace fds {

  class VolumeMeta {
 private:
    /*
     * TODO: This will eventually be replaced with the
     * actual volume metadata we get from OM.
     */
    std::string  vol_name;
    fds_uint64_t vol_uuid;

    /*
     * The volume catalog maintains mappings from
     * vol/blob/offset to object id.
     */
    VolumeCatalog *vcat;
    /*
     * The time catalog maintains pending changes to
     * the volume catalog.
     */
    TimeCatalog *tcat;

 public:
    /*
     * Default constructor should NOT be called
     * directly. It is needed for STL data struct
     * support.
     */
    VolumeMeta();
    VolumeMeta(const std::string& _name,
               fds_uint64_t _uuid);
    ~VolumeMeta();

    Error OpenTransaction(fds_uint32_t vol_offset,
                          const ObjectID &oid);
  };

}  // namespace fds

#endif  // SOURCE_DATA_MANAGER_VOLUMEMETA_H_
