///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_INCLUDE_CATALOGKEYS_OBJECTRANKKEY_H_
#define SOURCE_INCLUDE_CATALOGKEYS_OBJECTRANKKEY_H_

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKey.h"
#include "fds_types.h"

// Forward declarations.
namespace leveldb {

class Slice;

}  // namespace leveldb

namespace fds {

class ObjectRankKey : public CatalogKey
{
public:

    explicit ObjectRankKey (leveldb::Slice const& key);
    explicit ObjectRankKey (ObjectID const& objectId);

    ObjectID getObjectId () const;

protected:

    std::string getClassName () const override;

};

}  // namespace fds

#endif  // SOURCE_INCLUDE_CATALOGKEYS_OBJECTRANKKEY_H_
