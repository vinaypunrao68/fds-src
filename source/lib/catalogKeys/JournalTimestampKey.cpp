///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKeyType.h"

// Class include.
#include "JournalTimestampKey.h"

using std::string;

namespace fds {

JournalTimestampKey::JournalTimestampKey ()
        : CatalogKey{CatalogKeyType::JOURNAL_TIMESTAMP,
          std::string(CatalogKey::getNewDataSize(), '\0')}
{ }

string JournalTimestampKey::getClassName () const
{
    return "JournalTimestampKey";
}

}  // namespace fds
