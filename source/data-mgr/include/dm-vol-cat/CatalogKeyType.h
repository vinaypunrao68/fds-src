///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEYTYPE_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEYTYPE_H_

namespace fds {

enum class CatalogKeyType : unsigned char
{
    ///
    /// This should never appear on-disk or in memory. If encountered, it is always an error.
    ///
    ERROR = 0,

    ///
    /// FIXME: I don't know.
    ///
    JOURNAL_TIMESTAMP = 1,

    ///
    /// Volume metadata.
    ///
    VOLUME_METADATA = 2,

    ///
    /// Blob metadata. Key is just the blob name.
    ///
    BLOB_METADATA = 3,

    ///
    /// Objects the blob's data consists of.
    ///
    OBJECTS = 4,

    ///
    /// Reserved for future use.
    ///
    EXTENDED = 255
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEYTYPE_H_
