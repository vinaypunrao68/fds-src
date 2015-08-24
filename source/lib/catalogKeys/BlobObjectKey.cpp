///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Standard includes.
#include <stdexcept>
#include <string>
#include <utility>

// Internal includes.
#include "leveldb/db.h"
#include "CatalogKeyType.h"

// Class include.
#include "BlobObjectKey.h"

using std::invalid_argument;
using std::move;
using std::string;
using std::to_string;
using std::vector;

namespace fds {

BlobObjectKey::BlobObjectKey (leveldb::Slice const& leveldbKey)
        : CatalogKey{string(leveldbKey.data(), leveldbKey.size())}
{
    auto const dataLength = leveldbKey.size();
    if (dataLength < getNewDataSize())
    {
        throw invalid_argument{"Key of " + to_string(dataLength) + " bytes is not large enough to "
                               "be a BlobObjectKey."};
    }
}

BlobObjectKey::BlobObjectKey (string const& blobName)
        : BlobObjectKey{blobName, 0}
{ }

BlobObjectKey::BlobObjectKey (string const& blobName, fds_uint32_t objectIndex)
        : CatalogKey{CatalogKeyType::BLOB_OBJECTS,
                     string(CatalogKey::getNewDataSize(), '\0')
                     + string{reinterpret_cast<char*>(&objectIndex), sizeof(objectIndex)}
                     + blobName}
{ }

BlobObjectKey& BlobObjectKey::operator= (BlobObjectKey const& other)
{
    if (&other != this)
    {
        getData() = other.getData();
    }

    return *this;
}

BlobObjectKey& BlobObjectKey::operator= (BlobObjectKey&& other)
{
    if (&other != this)
    {
        getData() = move(other.getData());
    }

    return *this;
}

string BlobObjectKey::getBlobName () const
{
    auto& data = getData();
    return string{data.data() + getNewDataSize(), data.size() - getNewDataSize()};
}

fds_uint32_t BlobObjectKey::getObjectIndex () const
{
    return *reinterpret_cast<fds_uint32_t const*>(getData().data() + CatalogKey::getNewDataSize());
}

void BlobObjectKey::setBlobName (string const& value)
{
    auto& data = getData();
    data = data.substr(0, getNewDataSize()) + value;
}

void BlobObjectKey::setObjectIndex (fds_uint32_t value)
{
    getData().replace(CatalogKey::getNewDataSize(),
                      sizeof(value),
                      reinterpret_cast<char const*>(&value),
                      sizeof(value));
}

string BlobObjectKey::getClassName () const
{
    return "BlobObjectKey";
}

vector<string> BlobObjectKey::toStringMembers () const
{
    auto retval = CatalogKey::toStringMembers();

    retval.push_back("blobName: " + getBlobName());
    retval.push_back("objectIndex: " + getObjectIndex());

    return retval;
}

constexpr size_t BlobObjectKey::getNewDataSize ()
{
    return CatalogKey::getNewDataSize() + sizeof(fds_uint32_t);
}

}  // namespace fds
