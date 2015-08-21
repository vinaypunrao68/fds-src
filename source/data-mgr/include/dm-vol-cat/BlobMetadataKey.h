///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBMETADATAKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBMETADATAKEY_H_

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKey.h"

// Forward declarations.
namespace leveldb {

class Slice;

}  // namespace leveldb

namespace fds {

class BlobMetadataKey : public CatalogKey
{
public:

    explicit BlobMetadataKey (leveldb::Slice const& key);
    explicit BlobMetadataKey (std::string const& blobName);

    std::string const& getBlobName();

protected:

    std::string getClassName () const override;

};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_BLOBMETADATAKEY_H_
