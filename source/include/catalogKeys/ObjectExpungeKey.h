///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_INCLUDE_CATALOGKEYS_OBJECTEXPUNGEKEY_H_
#define SOURCE_INCLUDE_CATALOGKEYS_OBJECTEXPUNGEKEY_H_

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

};

}  // namespace fds

#endif  // SOURCE_INCLUDE_CATALOGKEYS_OBJECTEXPUNGEKEY_H_
