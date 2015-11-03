/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <blob/BlobTypes.h>
#include <string>
#include <utility>
#include <fds_uuid.h>

namespace fds {

std::ostream&
operator<<(std::ostream& out, const BlobOffsetPair& blobOffset) {
    out << "Blob " << blobOffset.first << " offset " << blobOffset.second;
    return out;
}

std::ostream&
operator<<(std::ostream& out, const BlobTxId& txId) {
    return out << txId.txId;
}

}  // namespace fds
