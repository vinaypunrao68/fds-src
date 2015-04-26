/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <dmchk.h>
#include <util/path.h>
#include <util/stringutils.h>
namespace fds {

DmChecker::DmChecker(int argc,
                     char *argv[],
                     const std::string & config,
                     const std::string & basePath,
                     Module *vec[],
                     const std::string &moduleName)
        : FdsProcess(argc, argv, config, basePath, "stdout", vec) {
}

Error DmChecker::loadVolume(fds_volid_t volumeUuid) {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::string volDir = util::strformat("%s/%ld", root->dir_sys_repo_dm().c_str(), volumeUuid);

    if (!util::dirExists(volDir)) {
        return ERR_VOL_NOT_FOUND;
    }
    
    volCat = boost::make_shared<DmVolumeCatalog>("DM checker");
    volDesc = boost::make_shared<VolumeDesc>(std::to_string(volumeUuid), volumeUuid);
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

Error DmChecker::listBlobs(bool fStatsOnly) {
    fpi::BlobDescriptorListType blobDescrList;
    Error err = volCat->listBlobs(volDesc->volUUID, &blobDescrList);
    if (!err.ok()) {
        std::cout << "error: " << err << volDesc->volUUID << std::endl;
        return err;
    }

    fds_uint32_t counter = 0;
    uint64_t  totalSize = 0;
    if (!fStatsOnly) {
        std::cout << "no.  : name   :  size  : numObjects" << std::endl;
    }

    uint64_t expectedObjectCount = 0;
    uint64_t seenObjectCount = 0;
    ObjectID objId;
    fpi::FDSP_BlobObjectList objList;
    blob_version_t blobVersion;
    fpi::FDSP_MetaDataList metaList;
    fds_uint64_t blobSize;

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

void DmChecker::blobInfo(const std::string& blobname) {
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
        std::cout << "unable to locate [" << blobname << "] in vol:" << volDesc->volUUID;
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

void DmChecker::getVolumeIds(std::vector<fds_volid_t>& vecVolumes) {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::vector<std::string> vecNames;

    util::getSubDirectories(root->dir_sys_repo_dm(), vecNames);

    for (const auto& name : vecNames) {
        vecVolumes.push_back(std::atoll(name.c_str()));
    }
    std::sort(vecVolumes.begin(), vecVolumes.end());
}

}  // namespace fds
