/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sstream>
#include <lib/Catalog.h>
#include <refcount/objectrefscanner.h>
#include <refcount/refcountmanager.h>
#include <DataMgr.h>
#include <counters.h>
#include <fdsp/sm_api_types.h>
#include <util/stringutils.h>
DECL_EXTERN_OUTPUT_FUNCS(ActiveObjectsMsg);
DECL_EXTERN_OUTPUT_FUNCS(SvcUuid);

namespace fds { namespace refcount {

RefCountManager::RefCountManager(DataMgr* dm) : dm(dm), Module("RefCountManager") {
    LOGNORMAL << "instantiating";
    transferContext.refCountManager = this;
    svcMgr = dm->getModuleProvider()->getSvcMgr();
    scanner.reset(new ObjectRefScanMgr(dm->getModuleProvider(), dm));
}

void RefCountManager::mod_startup() {
    scanner->mod_startup();
    scanner->setScanDoneCb(std::bind(&RefCountManager::scanDoneCb, this, PH_ARG1));
}

void RefCountManager::mod_shutdown() {
    scanner->mod_shutdown();
}

void RefCountManager::scanActiveObjects() {
    LOGDEBUG << "asking to scan active objects";
    scanner->scanOnce();
}

void RefCountManager::scanDoneCb(ObjectRefScanMgr*) {
    // now transfer the active objects to all SMs
    transferContext.volumeList.reset(new std::list<fds_volid_t>(scanner->getScanSuccessVols()));
    transferContext.currentToken = 0;
    transferContext.processNextToken();
}

void RefCountManager::objectFileTransferredCb(fds::net::FileTransferService::Handle::ptr handle,
                                              const Error& err,
                                              fds_token_id token) {

    if (!err.ok()) {
        LOGERROR << err
                 << " in transfer of object file [" << handle->destFile
                 << "] for token [" << token << "] "
                 << " for [" << transferContext.volumeList->size()
                 << "]";
        transferContext.tokenDone(token, handle->svcId, err);
        return;
    }

    LOGDEBUG << "object file [" << handle->destFile
             << "] for token [" << token << "] "
             << " for [" << transferContext.volumeList->size()
             << "] volumes has been transferred";
    // send the message
    fpi::ActiveObjectsMsgPtr msg(new fpi::ActiveObjectsMsg());
    auto request  =  svcMgr->getSvcRequestMgr()->newEPSvcRequest(handle->svcId);
    msg->filename = handle->destFile;
    msg->checksum = handle->getCheckSum();
    msg->scantimestamp = dm->counters->refscanLastRun.value();
    for (const auto& volId : *transferContext.volumeList) {
        msg->volumeIds.push_back(volId.get());
    }
    msg->token    = token;
    LOGDEBUG << "msg=" << msg;
    request->setPayload(FDSP_MSG_TYPEID(fpi::ActiveObjectsMsg), msg);
    request->onResponseCb(RESPONSE_MSG_HANDLER(RefCountManager::handleActiveObjectsResponse, token));
    request->invoke();
}

void RefCountManager::handleActiveObjectsResponse(fds_token_id token,
                                                  EPSvcRequest* request,
                                                  const Error& error,
                                                  SHPTR<std::string> payload) {
    if (error.ok()) {
        LOGDEBUG << "object file for token:" << token
                 << " has been accepted by " << request->getPeerEpId();
    } else {
        LOGERROR << "SM returned error: " << error
                 << " for token:" << token
                 << " reqid:" << request->getRequestId()
                 << " from:" << request->getPeerEpId();
    }
    transferContext.tokenDone(token, request->getPeerEpId(), error);
}

void RefCountManager::FileTransferContext::tokenDone(fds_token_id token, fpi::SvcUuid svcId, const Error& err) {
    if (token != currentToken) {
        LOGCRITICAL << "response received for unexpected token:" << token
                    << " expected:" << currentToken;
        return;
    }

    numResponsesToRecieve.decr();

    if (numResponsesToRecieve.get() == 0) {
        ++currentToken;
        if (!processNextToken()) {
            LOGNORMAL << "refscan and file transfers complete";
            // now send a done message to SM
            auto svcMgr = refCountManager->svcMgr;
            std::vector<fpi::SvcInfo> services;
            svcMgr->getSvcMap(services);
            fpi::GenericCommandMsgPtr msg(new fpi::GenericCommandMsg());
            msg->command="refscan.done";
            for (const auto& service : services) {
                if (service.svc_type != fpi::FDSP_STOR_MGR) continue;
                auto request  =  svcMgr->getSvcRequestMgr()->newEPSvcRequest(service.svc_id.svc_uuid);
                request->setPayload(FDSP_MSG_TYPEID(fpi::GenericCommandMsg), msg);
                request->invoke();
            }
        }
    }
}

bool RefCountManager::FileTransferContext::processNextToken() {
    std::string filename, tokenFileName;
    auto svcMgr = refCountManager->svcMgr;
    auto dlt = svcMgr->getCurrentDLT();
    auto myuuid = svcMgr->getSelfSvcUuid().svc_uuid;
    if (currentToken >= dlt->getNumTokens()) {
        LOGDEBUG << "no more tokens to process";
        refCountManager->scanner->setStateStopped();
        return false;
    }

    if (volumeList->empty()) {
        LOGDEBUG << "nothing to do";
        refCountManager->scanner->setStateStopped();
        return false;
    }

    filename = refCountManager->scanner->getTokenBloomfilterPath(currentToken);
    auto tokenGroup = dlt->getNodes(currentToken);
    numResponsesToRecieve.set(tokenGroup->getLength());

    if (filename.length() > 0) {
        LOGDEBUG << "will process active object file for token : " << currentToken;
        refCountManager->dm->counters->refscanNumTokenFiles.incr(1);
        tokenFileName = util::strformat("token_%d_%ld.bf", currentToken, myuuid);
        for (fds_uint32_t n = 0; n < tokenGroup->getLength(); n++) {
            refCountManager->dm->
                    fileTransfer->send(svcMgr->mapToSvcUuid(tokenGroup->get(n), fpi::FDSP_STOR_MGR ),
                                       filename,
                                       tokenFileName,
                                       std::bind(&RefCountManager::objectFileTransferredCb, refCountManager,
                                                 PH_ARG1, PH_ARG2, currentToken),
                                       false);
        }
    } else {
        LOGDEBUG << "no active object file for token : " << currentToken;
        fpi::ActiveObjectsMsgPtr msg(new fpi::ActiveObjectsMsg());
        msg->token    = currentToken;
        for (const auto& volId : *volumeList) {
            msg->volumeIds.push_back(volId.get());
        }
        msg->scantimestamp = refCountManager->dm->counters->refscanLastRun.value();
        LOGDEBUG << "msg=" << *msg;
        for (fds_uint32_t n = 0; n < tokenGroup->getLength(); n++) {
            auto svcId = svcMgr->mapToSvcUuid(tokenGroup->get(n), fpi::FDSP_STOR_MGR);
            auto request  =  refCountManager->svcMgr->getSvcRequestMgr()->newEPSvcRequest(svcId);
            request->setPayload(FDSP_MSG_TYPEID(fpi::ActiveObjectsMsg), msg);
            request->onResponseCb(std::bind(&RefCountManager::handleActiveObjectsResponse, refCountManager,
                                            currentToken, PH_ARG1, PH_ARG2, PH_ARG3));
            request->invoke();
        }
    }
    return true;
}

}  // namespace refcount
}  // namespace fds
