///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_VOLUMEMETADATAKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_VOLUMEMETADATAKEY_H_

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKey.h"

namespace fds {

class VolumeMetadataKey : public CatalogKey
{
public:

    VolumeMetadataKey ();

protected:

    std::string getClassName () const override;

};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_VOLUMEMETADATAKEY_H_
