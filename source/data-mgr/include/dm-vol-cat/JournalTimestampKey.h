///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_JOURNALTIMESTAMPKEY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_JOURNALTIMESTAMPKEY_H_

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKey.h"

namespace fds {

class JournalTimestampKey : public CatalogKey
{
public:

    JournalTimestampKey ();

protected:

    std::string getClassName () const override;

};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_JOURNALTIMESTAMPKEY_H_
