/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DmBlobTypes.h>

namespace fds {

/**
 * Constructs invalid version of a blob meta
 * until someone else actually assigns valid data
 */
BlobMetaDesc::BlobMetaDesc() {
    version = blob_version_invalid;
    vol_id = invalid_vol_id;
    blob_size = 0;
    meta_list.clear();
}

BlobMetaDesc::~BlobMetaDesc() {
}

}  // namespace fds
