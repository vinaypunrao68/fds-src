///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Internal includes.
#include "leveldb/db.h"
#include "CatalogKeyType.h"

// Class include.
#include "ObjectRankKey.h"

using std::string;

namespace fds {

ObjectRankKey::ObjectRankKey (leveldb::Slice const& key) : CatalogKey{string{key.data(),
                                                                             key.size()}}
{ }

ObjectRankKey::ObjectRankKey (ObjectID const& objectId)
        : CatalogKey{CatalogKeyType::OBJECT_RANK,
                     string{CatalogKey::getNewDataSize(), '\0'}
                     + string{reinterpret_cast<char const*>(objectId.GetId()), objectId.GetLen()}}
{ }

ObjectID ObjectRankKey::getObjectId () const
{
    return ObjectID{reinterpret_cast<uint8_t const*>(getData().data()
                                                     + CatalogKey::getNewDataSize()),
                    static_cast<fds_uint32_t>(getData().size() - CatalogKey::getNewDataSize())};
}

string ObjectRankKey::getClassName () const
{
    return "ObjectRankKey";
}

}  // namespace fds
