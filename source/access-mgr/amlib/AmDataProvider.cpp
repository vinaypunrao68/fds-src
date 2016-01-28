/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "AmDataProvider.h"
#include "AmRequest.h"

#include "fds_error.h"
#include "fds_types.h"

namespace fds
{

//
// This is utilized when you've got a request that the type has been lost during
// runtime and must be looked up again to get on the right response track
//
void
AmDataProvider::unknownTypeResume(AmRequest * amReq) {
    switch (amReq->io_type) {
        /* === Volume operations === */
        case fds::FDS_ATTACH_VOL:
            AmDataProvider::openVolume(amReq);
            break;
        case fds::FDS_DETACH_VOL:
            AmDataProvider::closeVolume(amReq);
            break;
        case fds::FDS_STAT_VOLUME:
            AmDataProvider::statVolume(amReq);
            break;
        case fds::FDS_SET_VOLUME_METADATA:
            AmDataProvider::setVolumeMetadata(amReq);
            break;
        case fds::FDS_GET_VOLUME_METADATA:
            AmDataProvider::getVolumeMetadata(amReq);
            break;
        case fds::FDS_VOLUME_CONTENTS:
            AmDataProvider::volumeContents(amReq);
            break;
        case fds::FDS_START_BLOB_TX:
            AmDataProvider::startBlobTx(amReq);
            break;
        case fds::FDS_COMMIT_BLOB_TX:
            AmDataProvider::commitBlobTx(amReq);
            break;
        case fds::FDS_ABORT_BLOB_TX:
            AmDataProvider::abortBlobTx(amReq);
            break;
        case fds::FDS_STAT_BLOB:
            AmDataProvider::statBlob(amReq);
            break;
        case fds::FDS_SET_BLOB_METADATA:
            AmDataProvider::setBlobMetadata(amReq);
            break;
        case fds::FDS_DELETE_BLOB:
            AmDataProvider::deleteBlob(amReq);
            break;
        case fds::FDS_RENAME_BLOB:
            AmDataProvider::renameBlob(amReq);
            break;
        case fds::FDS_GET_BLOB:
            AmDataProvider::getBlob(amReq);
            break;
        case fds::FDS_PUT_BLOB:
            AmDataProvider::putBlob(amReq);
            break;
        case fds::FDS_PUT_BLOB_ONCE:
            AmDataProvider::putBlobOnce(amReq);
            break;
        case fds::FDS_SM_PUT_OBJECT:
            AmDataProvider::putObject(amReq);
            break;
        case fds::FDS_CAT_UPD:
            AmDataProvider::updateCatalog(amReq);
            break;
        default:
            fds_panic("Unknown request type!");
            break;
    }
}

//
// This is utilized when you've got a request that the type has been lost during
// runtime and must be looked up again to get on the right response track
//
void
AmDataProvider::unknownTypeCb(AmRequest * amReq, Error const error) {
    switch (amReq->io_type) {
        /* === Volume operations === */
        case fds::FDS_ATTACH_VOL:
            AmDataProvider::openVolumeCb(amReq, error);
            break;
        case fds::FDS_DETACH_VOL:
            AmDataProvider::closeVolumeCb(amReq, error);
            break;
        case fds::FDS_STAT_VOLUME:
            AmDataProvider::statVolumeCb(amReq, error);
            break;
        case fds::FDS_SET_VOLUME_METADATA:
            AmDataProvider::setVolumeMetadataCb(amReq, error);
            break;
        case fds::FDS_GET_VOLUME_METADATA:
            AmDataProvider::getVolumeMetadataCb(amReq, error);
            break;
        case fds::FDS_VOLUME_CONTENTS:
            AmDataProvider::volumeContentsCb(amReq, error);
            break;
        case fds::FDS_START_BLOB_TX:
            AmDataProvider::startBlobTxCb(amReq, error);
            break;
        case fds::FDS_COMMIT_BLOB_TX:
            AmDataProvider::commitBlobTxCb(amReq, error);
            break;
        case fds::FDS_ABORT_BLOB_TX:
            AmDataProvider::abortBlobTxCb(amReq, error);
            break;
        case fds::FDS_STAT_BLOB:
            AmDataProvider::statBlobCb(amReq, error);
            break;
        case fds::FDS_SET_BLOB_METADATA:
            AmDataProvider::setBlobMetadataCb(amReq, error);
            break;
        case fds::FDS_DELETE_BLOB:
            AmDataProvider::deleteBlobCb(amReq, error);
            break;
        case fds::FDS_RENAME_BLOB:
            AmDataProvider::renameBlobCb(amReq, error);
            break;
        case fds::FDS_GET_BLOB:
            AmDataProvider::getBlobCb(amReq, error);
            break;
        case fds::FDS_PUT_BLOB:
            AmDataProvider::putBlobCb(amReq, error);
            break;
        case fds::FDS_PUT_BLOB_ONCE:
            AmDataProvider::putBlobOnceCb(amReq, error);
            break;
        case fds::FDS_SM_PUT_OBJECT:
            AmDataProvider::putObjectCb(amReq, error);
            break;
        case fds::FDS_CAT_UPD:
            AmDataProvider::updateCatalogCb(amReq, error);
            break;
        default:
            fds_panic("Unknown request type!");
            break;
    }
}


}  // namespace fds
