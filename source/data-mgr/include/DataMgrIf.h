/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DATAMGRIF_H_
#define SOURCE_DATA_MGR_INCLUDE_DATAMGRIF_H_
#include <string>
#include <sstream>

namespace fds {
/* Forward declarations */
class VolumeDesc;

/**
* @brief This interface primarily contains stubs that concrete DataMgr implements. This helps
* with creating Mock dm that can be used for mock testing.
* Move functions that need to be mocked in here
*/
struct DataMgrIf {
    virtual std::string getSysVolumeName(const fds_volid_t &volId) const
    {
        return "";
    }
    virtual std::string getSnapDirBase() const
    {
        return "";
    }
    virtual std::string getSnapDirName(const fds_volid_t &volId,
                                       const int64_t snapId) const
    {
        return "";
    }
    /* DO NOT ADD ANY MEMBERS */
};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DATAMGRIF_H_
