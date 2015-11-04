/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sstream>
#include <lib/Catalog.h>
#include <refcount/objectrefscanner.h>
#include <refcount/refcountmanager.h>
#include <DataMgr.h>
#include <fdsp/sm_api_types.h>
#include <util/stringutils.h>

namespace fds { namespace refcount {

RefCountManager::RefCountManager(DataMgr* dm) : dm(dm), Module("RefCountManager") {
    scanner.reset(new ObjectRefScanMgr(MODULEPROVIDER(), dm));
}

void RefCountManager::mod_startup() {
    scanner->mod_startup();
    scanner->setScanDoneCb(std::bind(&RefCountManager::scanDoneCb, this, ARG1));
}

void RefCountManager::mod_shutdown() {
    scanner->mod_shutdown();
}

void RefCountManager::scanDoneCb(ObjectRefScanMgr*) {
    LOGNORMAL << "ref scan callback";
    // now transfer the active objects to all SMs
    std::string filename, tokenFileName;
    auto svcMgr = MODULEPROVIDER()->getSvcMgr();
    auto dlt = svcMgr->getCurrentDLT();
    VolumeList volumeList (new std::list<fds_volid_t>(scanner->getScanSuccessVols()));
    for (fds_token_id token = 0; token < 256 ; token++) {
        filename = scanner->getTokenBloomfilterPath(token);
        if (filename.length() > 0) {
            auto tokenGroup = dlt->getNodes(token);
            tokenFileName = util::strformat("token_%d.bf", token);
            for (fds_uint32_t n = 0; n < tokenGroup->getLength(); n++) {
                dm->fileTransfer->send(svcMgr->mapToSvcUuid(tokenGroup->get(n), fpi::FDSP_STOR_MGR ),
                                       filename,
                                       tokenFileName, 
                                       std::bind(&RefCountManager::objectFileTransferredCb,this,
                                                 ARG1, ARG2, token, volumeList),
                                       false);
            }
        } else {
            LOGWARN << "no active object file for token : " << token;
        }
    }
}

void RefCountManager::objectFileTransferredCb(fds::net::FileTransferService::Handle::ptr handle,
                                              const Error& err,
                                              fds_token_id token,
                                              VolumeList volumeList) {

    LOGDEBUG << "object file [" << handle->destFile
             << "] for token [" << token << "] has been transferred";
    // send the message
    fpi::ActiveObjectsMsgPtr msg(new fpi::ActiveObjectsMsg());
    auto request  =  MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr()->newEPSvcRequest(handle->svcId);
    msg->filename = handle->destFile;
    msg->checksum = handle->getCheckSum();
    for (const auto& volId : *volumeList) {
        msg->volumeIds.push_back(volId.get());
    }
    msg->token    = token;
    request->setPayload(FDSP_MSG_TYPEID(fpi::ActiveObjectsMsg), msg);
    request->onResponseCb(RESPONSE_MSG_HANDLER(RefCountManager::handleActiveObjectsResponse, handle, token));
    request->invoke();
}

void RefCountManager::handleActiveObjectsResponse(fds::net::FileTransferService::Handle::ptr handle,
                                                  fds_token_id token,
                                                  EPSvcRequest* request,
                                                  const Error& error,
                                                  SHPTR<std::string> payload) {
    if (error.ok()) {
        LOGDEBUG << "object file for token:" << token << " has been accepted by " << SvcMgr::mapToSvcUuidAndName(handle->svcId);
    } else {
        LOGERROR << "SM returned error : " << error
                 << " on " << handle;
    }
}

}  // namespace refcount
}  // namespace fds
