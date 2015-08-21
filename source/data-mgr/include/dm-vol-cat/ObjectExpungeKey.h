///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_OBJECTEXPUNGEKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_OBJECTEXPUNGEKEY_H_

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKey.h"
#include "fds_types.h"
#include "fds_volume.h"

namespace fds {

class ObjectExpungeKey : public CatalogKey
{
public:

    explicit ObjectExpungeKey (leveldb::Slice const& key);
    ObjectExpungeKey (fds_volid_t const volumeId, ObjectID const objectId);

    ObjectID getObjectId () const;

    fds_volid_t getVolumeId () const;

protected:

    std::string getClassName () const override;

private:

    ObjectID _objectId;

    fds_volid_t _volumeId;

};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_OBJECTEXPUNGEKEY_H_
