///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_INCLUDE_CATALOGKEYS_CATALOGKEYTYPE_H_
#define SOURCE_INCLUDE_CATALOGKEYS_CATALOGKEYTYPE_H_

namespace fds {

enum class CatalogKeyType : unsigned char
{
    ///
    /// This should never appear on-disk or in memory. If encountered, it is always an error.
    ///
    ERROR = 0,

    ///
    /// Mark times of leveldb operations.
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
    /// Objects in a blob.
    ///
    BLOB_OBJECTS = 4,

    ///
    /// Object reference counts.
    ///
    OBJECT_EXPUNGE = 5,

    ///
    /// Object ranking for SSD.
    ///
    OBJECT_RANK = 6,

    ///
    /// Reserved for future use.
    ///
    EXTENDED = 255
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_CATALOGKEYS_CATALOGKEYTYPE_H_
