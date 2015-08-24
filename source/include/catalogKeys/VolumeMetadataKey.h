///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_INCLUDE_CATALOGKEYS_VOLUMEMETADATAKEY_H_
#define SOURCE_INCLUDE_CATALOGKEYS_VOLUMEMETADATAKEY_H_

// Standard includes.
#include <string>

// Internal includes.
#include "CatalogKey.h"

namespace fds {

class VolumeMetadataKey : public CatalogKey
{
public:

    VolumeMetadataKey ();

protected:

    std::string getClassName () const override;

};

}  // namespace fds

#endif  // SOURCE_INCLUDE_CATALOGKEYS_VOLUMEMETADATAKEY_H_
