///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEY_H_

// Standard includes.
#include <string>
#include <vector>

// Internal includes.
#include "leveldb/db.h"
#include "CatalogKeyType.h"

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

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEY_H_
