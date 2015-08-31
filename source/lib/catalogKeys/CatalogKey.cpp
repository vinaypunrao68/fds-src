///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Standard includes.
#include <utility>

// Internal includes.
#include "leveldb/db.h"
#include "util/stringutils.h"
#include "util/stringutils.tcc"

// Class include.
#include "CatalogKey.h"

using fds::util::join;
using std::move;
using std::string;
using std::vector;

namespace fds {

CatalogKeyType CatalogKey::getKeyType () const
{
    return *reinterpret_cast<CatalogKeyType const*>(_data.data());
}

CatalogKey::operator leveldb::Slice () const
{
    return _data;
}

string CatalogKey::toString () const
{
    return getClassName() + "(" + join<string, vector<string>>(toStringMembers(), ", ") + ")";
}

CatalogKey::CatalogKey (string&& data) : _data{move(data)}
{ }

CatalogKey::CatalogKey (CatalogKeyType keyType, string&& data) : CatalogKey{move(data)}
{
    if (_data.empty())
    {
        _data.push_back(*reinterpret_cast<char*>(&keyType));
    }
    else
    {
        _data[0] = *reinterpret_cast<char*>(&keyType);
    }
}

string const& CatalogKey::getData () const
{
    return _data;
}

string& CatalogKey::getData ()
{
    return _data;
}

vector<string> CatalogKey::toStringMembers () const
{
    string retval {"CatalogKeyType: "};

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch-enum"
    switch (getKeyType())
    {
    case CatalogKeyType::BLOB_METADATA: retval += "BLOB_METADATA"; break;
    case CatalogKeyType::BLOB_OBJECTS: retval += "BLOB_OBJECTS"; break;
    case CatalogKeyType::EXTENDED: retval += "EXTENDED"; break;
    case CatalogKeyType::JOURNAL_TIMESTAMP: retval += "JOURNAL_TIMESTAMP"; break;
    case CatalogKeyType::OBJECT_EXPUNGE: retval += "OBJECT_EXPUNGE"; break;
    case CatalogKeyType::OBJECT_RANK: retval += "OBJECT_RANK"; break;
    case CatalogKeyType::VOLUME_METADATA: retval += "VOLUME_METADATA"; break;
    case CatalogKeyType::ERROR:
    default:
        retval += "<ERROR>";
    }
#pragma GCC diagnostic pop

    return vector<string>{retval};
}

}  // namespace fds
