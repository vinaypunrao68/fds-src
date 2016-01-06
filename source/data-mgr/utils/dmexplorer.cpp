/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <dmexplorer.h>
#include <util/path.h>
#include <util/stringutils.h>
#include <DataMgr.h>

namespace fds {

DMExplorer::DMExplorer(int argc,
                     char *argv[],
                     const std::string & config,
                     const std::string & basePath,
                     Module *vec[],
                     const std::string &moduleName)
        : FdsProcess(argc, argv, config, basePath, "stdout", vec) {
}

Error DMExplorer::loadVolume(fds_volid_t volumeUuid) {
    const FdsRootDir* root = proc_fdsroot();
    std::string volDir = util::strformat("%s/%ld",
                                         root->dir_sys_repo_dm().c_str(),
                                         volumeUuid.get());

    if (!util::dirExists(volDir)) {
        return ERR_VOL_NOT_FOUND;
    }

    volCat = boost::make_shared<DmVolumeCatalog>(this, "DM checker");
    volDesc = boost::make_shared<VolumeDesc>(std::to_string(volumeUuid.get()), volumeUuid);
    // TODO(Andrew): We're just making up a max object size because the catalog add
    // is going to expect it. For basic blob traversal, it's not used so doesn't matter.
    // Ideally, DM's catalog superblock could describe the volume and we could pull that
    // data from there
    volDesc->maxObjSizeInBytes = 2 * 1024 * 1024;

    Error err = volCat->addCatalog(*volDesc);
    if (!err.ok()) return err;
    err = volCat->activateCatalog(volDesc->volUUID);
    if (!err.ok()) return err;
    return ERR_OK;
}

Error DMExplorer::listBlobs(bool fStatsOnly) {
    fpi::BlobDescriptorListType blobDescrList;
    Error err = volCat->listBlobs(volDesc->volUUID, &blobDescrList);
    if (!err.ok()) {
        std::cout << "error: " << err << volDesc->volUUID << std::endl;
        return err;
    }

    fds_uint32_t counter = 0;
    uint64_t  totalSize = 0;

    uint64_t expectedObjectCount = 0;
    uint64_t seenObjectCount = 0;
    ObjectID objId;
    fpi::FDSP_BlobObjectList objList;
    blob_version_t blobVersion;
    fpi::FDSP_MetaDataList metaList;
    fds_uint64_t blobSize;

    if (!fStatsOnly && !blobDescrList.empty()) {
        std::cout << "no.  : name   :  size  : numObjects" << std::endl;
    }


    for (const auto &it : blobDescrList) {
        counter++;
        objList.clear();
        expectedObjectCount = std::ceil(it.byteCount*1.0/ volDesc->maxObjSizeInBytes);
        if (!fStatsOnly) {
            std::cout << counter << " : " << it.name
                      << " : " << it.byteCount << " bytes"
                      << " : " << expectedObjectCount
                      << std::endl;
            // verify if this has the correct no.of objects in the blob

            volCat->getBlob(volDesc->volUUID,
                            it.name, 0, expectedObjectCount*volDesc->maxObjSizeInBytes,
                            &blobVersion, &metaList, &objList, &blobSize);

            if (expectedObjectCount != objList.size()) {
                std::cout << "error : num objects"
                          << "[" <<  objList.size() << "]"
                          << " does not match expected"
                          << "[" << expectedObjectCount<< "]" << std::endl;
            }
        }
        totalSize += it.byteCount;
    }
    if (!fStatsOnly) {
        std::cout << std::endl;
    }
    std::cout << "vol:" << volDesc->volUUID
              << " numblobs: " << counter
              << " totalSize: " << totalSize << std::endl;
    return ERR_OK;
}

void DMExplorer::blobInfo(const std::string& blobname) {
    uint64_t expectedObjectCount = 0;
    uint64_t seenObjectCount = 0;
    ObjectID objId;
    fpi::FDSP_BlobObjectList objList;
    blob_version_t blobVersion;
    fpi::FDSP_MetaDataList metaList;
    fds_uint64_t blobSize;
    Error err;

    err = volCat->getBlobMeta(volDesc->volUUID, blobname, &blobVersion, &blobSize, &metaList);
    if (!err.ok()) {
        std::cout << "unable to locate [" << blobname << "] in vol:" << volDesc->volUUID << std::endl;
        return;
    }
    expectedObjectCount = std::ceil(blobSize*1.0/ volDesc->maxObjSizeInBytes);
    metaList.clear();
    err = volCat->getBlob(volDesc->volUUID,
                          blobname, 0, expectedObjectCount* volDesc->maxObjSizeInBytes,
                          &blobVersion, &metaList, &objList, &blobSize);

    fds_uint32_t counter = 0;
    std::cout << "Meta Data::::" << std::endl;
    std::cout << "  name : " << blobname << std::endl;
    std::cout << "  size : " << blobSize << std::endl;
    for (const auto& meta : metaList) {
        std::cout << "  " << meta.key << " : "  << meta.value << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Objects in blob::::" << std::endl;
    for (const auto& obj : objList) {
        objId.SetId(obj.data_obj_id.digest);
        std::cout << ++counter << " : " << objId.ToString() << std::endl;
    }
}

Error DMExplorer::listBlobsWithObject(std::string strObjId) {
    fpi::BlobDescriptorListType blobDescrList;
    std::string digest;
    Error err = volCat->listBlobs(volDesc->volUUID, &blobDescrList);
    if (!err.ok()) {
        std::cout << "error: " << err << volDesc->volUUID << std::endl;
        return err;
    }

    fds_uint32_t counter = 0, numOccurInBlob = 0, numOccur = 0;
    uint64_t  totalSize = 0;

    uint64_t expectedObjectCount = 0;
    uint64_t seenObjectCount = 0;
    ObjectID objId, objId2;
    fpi::FDSP_BlobObjectList objList;
    blob_version_t blobVersion;
    fpi::FDSP_MetaDataList metaList;
    fds_uint64_t blobSize;

    if (strObjId.find("0x",0,2) == std::string::npos) {
        strObjId.insert(0,"0x");
    }
    objId = strObjId;
    digest = std::string((const char *)objId.GetId(), objId.getDigestLength());

    for (const auto &it : blobDescrList) {
        objList.clear();
        expectedObjectCount = std::ceil(it.byteCount*1.0/ volDesc->maxObjSizeInBytes);

        volCat->getBlob(volDesc->volUUID,
                        it.name, 0, expectedObjectCount*volDesc->maxObjSizeInBytes,
                        &blobVersion, &metaList, &objList, &blobSize);

        numOccurInBlob = 0;
        for (const auto& obj : objList) {
            if (obj.data_obj_id.digest == digest) numOccurInBlob++;
        }

        if (numOccurInBlob) {
            counter++;
            numOccur += numOccurInBlob;
            std::cout << it.name << " : " << numOccurInBlob << std::endl;
        }
    }
    if (numOccur) {
        std::cout << "vol:" << volDesc->volUUID
                  << " in [" << counter << "] blobs "
                  << " occur:" << numOccur << std::endl;
    }
    return err;
}

void DMExplorer::getVolumeIds(std::vector<fds_volid_t>& vecVolumes) {
    const FdsRootDir* root = proc_fdsroot();
    dmutil::getVolumeIds(root, vecVolumes);
}

}  // namespace fds
