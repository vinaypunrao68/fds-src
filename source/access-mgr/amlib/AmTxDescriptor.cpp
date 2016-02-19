/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include "AmTxDescriptor.h"

namespace fds {

AmTxDescriptor::AmTxDescriptor(fds_volid_t volUuid,
                               const BlobTxId &id,
                               fds_uint64_t dmtVer,
                               const std::string &name,
                               const fds_int32_t mode)
        : volId(volUuid),
          txId(id),
          originDmtVersion(dmtVer),
          blobName(name),
          blob_mode(mode),
          opType(FDS_PUT_BLOB),
          stagedBlobDesc(new BlobDescriptor()) {
    stagedBlobDesc->setVolId(volId.get());
    stagedBlobDesc->setBlobName(blobName);
    stagedBlobDesc->setBlobSize(0);
    // TODO(Andrew): We're leaving the blob version unset
    // We'll need to revist when we do versioning
    // TODO(Andrew): We default the op type to PUT, but it's
    // conceivable to PUT and DELETE in the same transaction,
    // so we really want to mask the op type.
}

}  // namespace fds
