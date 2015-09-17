///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKeyType.h"

// Class include.
#include "VolumeMetadataKey.h"

using std::string;

namespace fds {

VolumeMetadataKey::VolumeMetadataKey ()
        : CatalogKey{CatalogKeyType::VOLUME_METADATA, string{CatalogKey::getNewDataSize(), '\0'}}
{ }

string VolumeMetadataKey::getClassName () const
{
    return "VolumeMetadataKey";
}

}  // namespace fds
