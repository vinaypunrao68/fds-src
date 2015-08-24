///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_INCLUDE_CATALOGKEYS_CATALOGKEY_H_
#define SOURCE_INCLUDE_CATALOGKEYS_CATALOGKEY_H_

// Standard includes.
#include <string>
#include <vector>

// Internal includes.
#include "CatalogKeyType.h"

// Forward declarations.
namespace leveldb {

class Slice;

}  // namespace leveldb

namespace fds {

class CatalogKey
{
public:

    CatalogKeyType getKeyType () const;

    virtual operator leveldb::Slice () const;

    virtual std::string toString () const;

protected:

    explicit CatalogKey (std::string&& data);

    CatalogKey (CatalogKeyType keyType, std::string&& data);

    virtual ~CatalogKey () = default;

    virtual std::string getClassName () const = 0;

    std::string const& getData () const;
    std::string& getData ();

    virtual std::vector<std::string> toStringMembers () const;

    static constexpr size_t getNewDataSize ();

private:

    std::string _data;

};

constexpr size_t CatalogKey::getNewDataSize ()
{
    return sizeof(CatalogKeyType);
}

}  // namespace fds

#endif  // SOURCE_INCLUDE_CATALOGKEYS_CATALOGKEY_H_
