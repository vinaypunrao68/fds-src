///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Internal includes.
#include "CatalogKeyType.h"
#include "leveldb/db.h"

// Class include.
#include "BlobMetadataKey.h"

using std::string;
using std::vector;

namespace fds {

BlobMetadataKey::BlobMetadataKey (leveldb::Slice const& key) : CatalogKey{string{key.data(),
                                                                                 key.size()}}
{ }

BlobMetadataKey::BlobMetadataKey (string const& blobName)
        : CatalogKey{CatalogKeyType::BLOB_METADATA,
                     string(CatalogKey::getNewDataSize(), '\0') + blobName}
{ }

string BlobMetadataKey::getBlobName () const
{
    return string(getData().data() + CatalogKey::getNewDataSize(),
                  getData().size() - CatalogKey::getNewDataSize());
}

string BlobMetadataKey::getClassName () const
{
    return "BlobMetadataKey";
}

vector<string> BlobMetadataKey::toStringMembers () const
{
    auto retval = CatalogKey::toStringMembers();

    retval.emplace_back("blobName: " + getBlobName());

    return retval;
}

}  // namespace fds
