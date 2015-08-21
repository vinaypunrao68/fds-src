///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBOBJECTKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBOBJECTKEY_H_

// Standard includes.
#include <string>
#include <vector>

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
    BlobObjectKey (std::string const& blobName, fds_uint32_t objectIndex);

    BlobObjectKey& operator= (BlobObjectKey const& other);
    BlobObjectKey& operator= (BlobObjectKey&& other);

    std::string getBlobName () const;

    fds_uint32_t getObjectIndex () const;

    void setBlobName (std::string const& value);

    void setObjectIndex (fds_uint32_t value);

protected:

    std::string getClassName () const override;

    std::vector<std::string> toStringMembers () const override;

    static constexpr size_t getNewDataSize ();
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBOBJECTKEY_H_
