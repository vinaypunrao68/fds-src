/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Basic include information for a volume.
 */

#ifndef SOURCE_INCLUDE_FDS_VOLUME_H_
#define SOURCE_INCLUDE_FDS_VOLUME_H_

#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>

#include "include/fds_types.h"

namespace fds {

  typedef fds_uint64_t fds_volid_t;
  typedef boost::posix_time::ptime ptime;

  const fds_volid_t invalid_vol_id = 0;

  /*
   * Basic descriptor class for a volume. This is intended
   * to be a cacheable/passable description of a volume instance.
   * The authoritative volume information is stored on the OM.
   */
  class VolumeDesc {
 private:
    /*
     * Basic ID information.
     */
    std::string name;
    fds_volid_t uuid;

    fds_uint64_t parent_ld; /* TODO: Fill with parent local domain ID */

    /* TODO: Put permission info here. */
    /* TODO: Put some SLA info here. */
    /* TODO: Volume type */

    ptime ctime; /* Create time */

 public:
    /*
     * Constructors/destructors
     */
    VolumeDesc(const std::string& _name, fds_volid_t _uuid)
        : name(_name),
        uuid(_uuid),
        parent_ld(0) {
      assert(_uuid != invalid_vol_id);
    }
    ~VolumeDesc() {
    }

    std::string GetName() const {
      return name;
    }

    fds_volid_t GetID() const {
      return uuid;
    }

    bool operator==(const VolumeDesc &rhs) const {
      return (this->uuid == rhs.uuid);
    }

    bool operator!=(const VolumeDesc &rhs) const {
      return !(*this == rhs);
    }
  };

  inline std::ostream& operator<<(std::ostream& out,
                                  const VolumeDesc& vol_desc) {
    return out << "Vol<" << vol_desc.GetName() << ":"
               << vol_desc.GetID() << ">";
  }
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_VOLUME_H_
