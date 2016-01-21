/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMTXDESCRIPTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMTXDESCRIPTOR_H_

#include <string>
#include <unordered_map>

#include "AmAsyncDataApi.h"
#include "shared/fds_types.h"
#include "blob/BlobTypes.h"

namespace fds
{

/**
 * Descriptor for an AM blob transaction. Contains information
 * about the transaction and a staging area for the updates.
 */
class AmTxDescriptor {
  public:
    /// Blob trans ID
    BlobTxId txId;
    /// DMT version when transaction started
    fds_uint64_t originDmtVersion;
    /// Blob being mutated
    std::string blobName;
    /// Volume context for the transaction
    fds_volid_t volId;

    /// Type of the staged transaction
    fds_io_op_t opType;
    /// Staged blob descriptor for the transaction
    BlobDescriptor::ptr stagedBlobDesc;
    /// Staged blob offsets updates
    typedef std::unordered_map<BlobOffsetPair, std::pair<ObjectID, uint64_t>,
                               BlobOffsetPairHash> BlobOffsetMap;
    BlobOffsetMap stagedBlobOffsets;
    /// Staged blob object updates for the transaction
    typedef std::unordered_map<ObjectID, boost::shared_ptr<std::string>,
                               ObjectHash> BlobObjectMap;
    BlobObjectMap stagedBlobObjects;

    AmTxDescriptor(fds_volid_t volUuid,
                   const BlobTxId &id,
                   fds_uint64_t dmtVer,
                   const std::string &name);
    AmTxDescriptor(AmTxDescriptor const& rhs) = delete;
    AmTxDescriptor& operator=(AmTxDescriptor const& rhs) = delete;
    ~AmTxDescriptor() = default;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMTXDESCRIPTOR_H_
