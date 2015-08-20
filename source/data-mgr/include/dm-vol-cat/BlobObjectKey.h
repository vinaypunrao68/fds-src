///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBOBJECTKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBOBJECTKEY_H_

// Standard includes.
#include <string>

// Internal includes.
#include "leveldb/db.h"
#include "CatalogKey.h"
#include "fds_types.h"

namespace fds {

class BlobObjectKey : public CatalogKey
{
public:

    explicit BlobObjectKey (leveldb::Slice const& leveldbKey);
    explicit BlobObjectKey (std::string const& blobName);
    explicit BlobObjectKey (std::string&& blobName);
    BlobObjectKey (std::string const& blobName, fds_uint32_t objectIndex);
    BlobObjectKey (std::string&& blobName, fds_uint32_t objectIndex);

    BlobObjectKey& operator= (BlobObjectKey const& other);
    BlobObjectKey& operator= (BlobObjectKey&& other);

    operator leveldb::Slice () const override;

    std::string const& getBlobName ();

    fds_uint32_t getObjectIndex ();

    void setBlobName (std::string const& value);
    void setBlobName (std::string&& value);

    void setObjectIndex (fds_uint32_t value);

private:

    std::string blobName;

    fds_uint32_t objectIndex;

};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBOBJECTKEY_H_
