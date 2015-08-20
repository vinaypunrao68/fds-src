///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEY_H_

// Internal includes.
#include "leveldb/db.h"
#include "CatalogKeyType.h"

namespace fds {

class CatalogKey
{
public:

    CatalogKeyType getKeyType ();

    virtual operator leveldb::Slice () const;

    virtual std::string toString () const;

protected:

    explicit CatalogKey (CatalogKeyType keyType);

    virtual ~CatalogKey () = default;

private:

    CatalogKeyType _keyType;

};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEY_H_
