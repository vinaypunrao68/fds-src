///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Internal includes.
#include "leveldb/db.h"
#include "CatalogKeyType.h"

// Class include.
#include "ObjectExpungeKey.h"

using std::string;
using std::to_string;
using std::vector;

namespace fds {

ObjectExpungeKey::ObjectExpungeKey (leveldb::Slice const& key) : CatalogKey{string{key.data(),
                                                                                   key.size()}}
{ }

ObjectExpungeKey::ObjectExpungeKey (fds_volid_t const volumeId, ObjectID const objectId)
        : CatalogKey{CatalogKeyType::OBJECT_EXPUNGE,
                     string{CatalogKey::getNewDataSize(), '\0'}
                     + string{reinterpret_cast<char const*>(volumeId.get()),
                              sizeof(fds_volid_t::value_type)}
                     + string{reinterpret_cast<char const*>(objectId.GetId()), objectId.GetLen()}}
{ }

ObjectID ObjectExpungeKey::getObjectId () const
{
    return ObjectID{reinterpret_cast<uint8_t const*>(getData().data()
                                                     + CatalogKey::getNewDataSize()),
                    sizeof(fds_volid_t::value_type)};
}

fds_volid_t ObjectExpungeKey::getVolumeId () const
{
    return fds_volid_t{*reinterpret_cast<fds_volid_t::value_type const*>(
            getData().data() + CatalogKey::getNewDataSize() + sizeof(fds_volid_t::value_type))};
}

string ObjectExpungeKey::getClassName () const
{
    return "ObjectExpungeKey";
}

vector<string> ObjectExpungeKey::toStringMembers () const
{
    auto retval = CatalogKey::toStringMembers();

    retval.emplace_back("volumeId: " + to_string(getVolumeId().get()));
    retval.emplace_back("objectId: " + getObjectId().ToHex());

    return retval;
}

}  // namespace fds
