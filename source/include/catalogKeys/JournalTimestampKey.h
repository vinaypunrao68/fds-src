///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_INCLUDE_CATALOGKEYS_JOURNALTIMESTAMPKEY_H_
#define SOURCE_INCLUDE_CATALOGKEYS_JOURNALTIMESTAMPKEY_H_

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

#endif  // SOURCE_INCLUDE_CATALOGKEYS_JOURNALTIMESTAMPKEY_H_
