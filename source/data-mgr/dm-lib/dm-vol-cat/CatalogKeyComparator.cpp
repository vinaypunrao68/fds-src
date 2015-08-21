///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Standard includes.
#include <stdexcept>

// Internal includes.
#include "dm-vol-cat/BlobMetadataKey.h"
#include "dm-vol-cat/BlobObjectKey.h"
#include "dm-vol-cat/CatalogKeyType.h"
#include "dm-vol-cat/ObjectExpungeKey.h"

// Class include.
#include "dm-vol-cat/CatalogKeyComparator.h"
#include "dm-vol-cat/CatalogKeyComparator.tcc"

using std::domain_error;
using std::invalid_argument;
using std::string;
using std::to_string;
using leveldb::Slice;

namespace fds {

int CatalogKeyComparator::Compare (Slice const& lhs, Slice const& rhs) const
{
    if (lhs.empty()) throw invalid_argument("lhs has no data.");
    if (rhs.empty()) throw invalid_argument("rhs has no data.");

    auto lhsKeyType = *reinterpret_cast<CatalogKeyType const*>(lhs.data());
    auto rhsKeyType = *reinterpret_cast<CatalogKeyType const*>(rhs.data());

    if (lhsKeyType == CatalogKeyType::ERROR) throw invalid_argument("lhs has key type ERROR.");
    if (rhsKeyType == CatalogKeyType::ERROR) throw invalid_argument("rhs has key type ERROR.");

    auto keyTypeResult = _compareWithOperators(lhsKeyType, rhsKeyType);
    if (keyTypeResult == 0)
    {
        switch (lhsKeyType)
        {
        case CatalogKeyType::BLOB_METADATA:
        {
            BlobMetadataKey typedLhs { lhs };
            BlobMetadataKey typedRhs { rhs };

            return _compareWithOperators(typedLhs.getBlobName(), typedRhs.getBlobName());
        }

        case CatalogKeyType::BLOB_OBJECTS:
        {
            BlobObjectKey typedLhs { lhs };
            BlobObjectKey typedRhs { rhs };

            int blobNameResult = _compareWithOperators(typedLhs.getBlobName(),
                                                       typedRhs.getBlobName());
            if (blobNameResult == 0)
            {
                return _compareWithOperators(typedLhs.getObjectIndex(), typedRhs.getObjectIndex());
            }
            else
            {
                return blobNameResult;
            }
        }

        case CatalogKeyType::EXTENDED:
            throw domain_error("EXTENDED key type is unsupported.");

        case CatalogKeyType::JOURNAL_TIMESTAMP:
            return 0;

        case CatalogKeyType::OBJECT_EXPUNGE:
        {
            ObjectExpungeKey typedLhs { lhs };
            ObjectExpungeKey typedRhs { rhs };

            int volumeIdResult = _compareWithOperators(typedLhs.getVolumeId(),
                                                       typedRhs.getVolumeId());
            if (volumeIdResult == 0)
            {
                return _compareWithOperators(typedLhs.getObjectId(), typedRhs.getObjectId());
            }
            else
            {
                return volumeIdResult;
            }
        }

        case CatalogKeyType::VOLUME_METADATA:
            return 0;

        default:
            throw domain_error("Key type " + to_string(static_cast<unsigned int>(lhsKeyType))
                               + " is unsupported.");
        }
    }
    else
    {
        return keyTypeResult;
    }
}

void CatalogKeyComparator::FindShortSuccessor (string* key) const
{
    // No-op.
}

void CatalogKeyComparator::FindShortestSeparator (string* start, Slice const& limit) const
{
    // No-op.
}

char const* CatalogKeyComparator::Name () const
{
    return "CatalogKeyComparator";
}

}  // namespace fds
