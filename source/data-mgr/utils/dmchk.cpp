/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <dmchk.h>

namespace fds {

DmChecker::DmChecker(int argc,
                     char *argv[],
                     const std::string & config,
                     const std::string & basePath,
                     Module *vec[],
                     const std::string &moduleName,
                     fds_volid_t volumeUuid)
        : FdsProcess(argc, argv, config, basePath, vec) {
    volCat = boost::make_shared<DmVolumeCatalog>(moduleName.c_str());
    volDesc = boost::make_shared<VolumeDesc>(std::to_string(volumeUuid), volumeUuid);
    // TODO(Andrew): We're just making up a max object size because the catalog add
    // is going to expect it. For basic blob traversal, it's not used so doesn't matter.
    // Ideally, DM's catalog superblock could describe the volume and we could pull that
    // data from there
    volDesc->maxObjSizeInBytes = 2 * 1024 * 1024;

    Error err = volCat->addCatalog(*volDesc);
    fds_verify(err.ok());
    err = volCat->activateCatalog(volDesc->volUUID);
    fds_verify(err.ok());
}

void
DmChecker::listBlobs() {
    fpi::BlobDescriptorListType blobDescrList;
    Error err = volCat->listBlobs(volDesc->volUUID, &blobDescrList);
    if (!err.ok()) {
        LOGNORMAL << "Unable to list blobs for volume " << volDesc->volUUID;
        return;
    }

    fds_uint32_t counter = 0;
    for (const auto &it : blobDescrList) {
        std::cout << "Blob " << counter++ << ": " << it.name
                  << ", " << it.byteCount << " bytes" << std::endl;
    }
    std::cout << "Listed all " << counter << " blobs" << std::endl;
}

}  // namespace fds
