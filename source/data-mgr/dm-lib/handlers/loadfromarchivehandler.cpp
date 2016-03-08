/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <util/path.h>
#include <fds_error.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
// incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>
#include <VolumeMeta.h>
#include <boost/filesystem.hpp>
DECL_EXTERN_OUTPUT_FUNCS (LoadFromArchiveMsg)

namespace fds {
namespace dm {

LoadFromArchiveHandler::LoadFromArchiveHandler(DataMgr& dataManager) : Handler(dataManager) {
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::LoadFromArchiveMsg, handleRequest);
    }
}

void LoadFromArchiveHandler::handleRequest(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                           SHPTR<fpi::LoadFromArchiveMsg>& message) {
    LOGDEBUG << asyncHdr << " - " << message;

    // Handle U-turn
    HANDLE_U_TURN();

    auto dmReq = new DmIoLoadFromArchive(message);
    dmReq->cb = BIND_MSG_CALLBACK(LoadFromArchiveHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void LoadFromArchiveHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoLoadFromArchive* typedRequest = static_cast<DmIoLoadFromArchive*>(dmRequest);

    LOGDEBUG << "Will reload volume " << *typedRequest;

    fds_volid_t volId(typedRequest->message->volId);

    // wait for the volume to be active upto 5 minutes
    // This is done here , because LFA request could come in for a clone even before 
    // an addVolume could reach the DM ..
    int count = 600;
    bool fVolumeReady = false;
    while (count > 0 && !fVolumeReady) {
        const auto vol = dataManager.getVolumeDesc(volId);
        if (vol != nullptr) {
            fVolumeReady = (vol->isStateActive() || vol->isStateOffline());
        }
        if (!fVolumeReady) {
            if (count % 60 == 0) {
                LOGWARN << "vol:" << volId << " not ready yet";
            }
            usleep(500000);  // 0.5s
        }
        count --;
    }

    const VolumeDesc * voldesc = dataManager.getVolumeDesc(volId);
    if (!voldesc) {
        LOGERROR << "Volume entry not found in descriptor map for vol:" << volId;
        helper.err = ERR_VOL_NOT_FOUND;
    } else {
        
        std::string archiveFileName = util::strformat("%ld.tgz",volId.get());
        std::string archiveFile = dataManager.fileTransfer->getFullPath(archiveFileName);

        if (!util::fileExists(archiveFile)) {
            LOGERROR << "unable to locate archive file to load: " << archiveFile;
            helper.err = ERR_VOL_NOT_FOUND;
        } else {
            // untar the archive file
            std::string dbDir = dmutil::getVolumeDir(NULL, volId);
            std::ostringstream oss;
            oss << "tar -zxvf " << archiveFile << " --directory=" << dbDir;

            LOGDEBUG << "about to exec:[" << oss.str() << "]";
            auto exitCode = std::system(oss.str().c_str());
            if (exitCode != 0) {
                LOGERROR << "unable to untar vol-db:[" << oss.str() << "]"
                         << " exit:" << exitCode;
                helper.err = ERR_NOT_FOUND;
            }

            if (helper.err.ok()) {
                helper.err = dataManager.timeVolCat_->queryIface()->reloadCatalog(*voldesc);
            }
         
            // remove archive file
            LOGDEBUG << "removing db-archive: " << archiveFile;
            boost::filesystem::remove(archiveFile);
        }        
    }

    if (helper.err.ok()) {
        LOGNORMAL << "initing seqid and stuff";
        auto volMeta = dataManager.getVolumeMeta(volId);
        if (volMeta != nullptr) {
            Error err = volMeta->initState();
            if (!err.ok()) {
                LOGERROR << "vol:" << volId << " init vol state failed";
            }
        } else {
            LOGERROR << "vol:" << volId << " unable to locate volMeta";
        }
    }

    if (!helper.err.ok()) {
        LOGERROR << "error: " << helper.err;
        PerfTracer::incr(typedRequest->opReqFailedPerfEventType,
                         typedRequest->getVolId());
    }
}

void LoadFromArchiveHandler::handleResponse(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                            SHPTR<fpi::LoadFromArchiveMsg>& message,
                                            Error const& e, DmRequest* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    LOGDEBUG << "Reload volume completed " << e << " " << logString(*asyncHdr);

    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
