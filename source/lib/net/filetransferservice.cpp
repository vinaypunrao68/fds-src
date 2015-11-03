/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fstream>

#include <fdsp/svc_types_types.h>
#include <fdsp/svc_api_types.h>
#include <net/filetransferservice.h>
#include <util/Log.h>
#include <util/path.h>

#include <net/SvcRequest.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>
#include <net/PlatNetSvcHandler.h>


#define FT_CHUNKSIZE 2*MB
#define INVALID_OFFSET -1

namespace fds { namespace net {

fds_uint64_t getHashCode(const fpi::SvcUuid &svcId, const std::string& srcFile) {
    static std::hash<std::string> getStrHash;
    return getStrHash(srcFile) + svcId.svc_uuid;
}

FileTransferHandle::FileTransferHandle(const std::string& srcFile,
                                       const std::string& destFile,
                                       const fpi::SvcUuid& svcId,
                                       FileTransferService* ftService) : srcFile(srcFile),
                                                                         destFile(destFile),
                                                                         svcId(svcId),
                                                                         ftService(ftService) {
    checkSum = util::getFileChecksum(srcFile);
    is.open(srcFile.c_str(), std::ifstream::binary);
    if (!is) {
        GLOGERROR << "unable to open file [" << srcFile << "]";
        return;
    }
    is.seekg (0, is.end);
    fileSize = is.tellg();
    is.seekg (0, is.beg);

    GLOGDEBUG << "[file:" << srcFile << " sz: " << fileSize << " chksum:" << checkSum << "]";
}

const std::string& FileTransferHandle::getCheckSum() const {
    return checkSum;
}

void FileTransferHandle::done(const Error& error) {
    ftService->done(getHashCode(), error);
}

bool FileTransferHandle::hasMoreData() const {
    return totalDataRead < fileSize;
}

bool FileTransferHandle::hasStarted() const {
    return (lastOffset >= 0);
}

bool FileTransferHandle::isComplete() const {
    return hasStarted() && (totalDataRead == fileSize);
}

bool FileTransferHandle::getNextChunk(std::string& data) {
    GLOGDEBUG << "here : " << (*this);
    data.reserve(FT_CHUNKSIZE);
    data.clear();
    char * buffer = new char [FT_CHUNKSIZE];
    bool fReturn = true;
    lastOffset = totalDataRead;
    is.read(buffer, FT_CHUNKSIZE);
    if (is.bad()) {
        fReturn = false;
    } else {
        data.assign(buffer, is.gcount());
        totalDataRead += is.gcount();
    }
    delete[] buffer;
    return fReturn;
}

fds_uint64_t FileTransferHandle::getHashCode() const {
    return fds::net::getHashCode(svcId, srcFile);
}

FileTransferHandle::~FileTransferHandle(){
    is.close();
}

bool FileTransferService::exists(fds_uint64_t hashCode) {
    return transferMap.end() != transferMap.find(hashCode);
}

FileTransferHandle::ptr FileTransferService::get(fds_uint64_t hashCode) {
    if (!exists(hashCode)) return NULL;
    return transferMap.at(hashCode);
}

FileTransferService::FileTransferService(const std::string& destDir, SvcMgr* svcMgr_) : svcMgr(svcMgr_)  {
    if (destDir.rfind('/') != destDir.length() - 1) {
        this->destDir = destDir + "/";
    } else {
        this->destDir = destDir;
    }
    FdsRootDir::fds_mkdir(destDir.c_str());
    GLOGNORMAL << "file transfer dest dir : " << destDir;

    // register handlers
    if (svcMgr == NULL) svcMgr = MODULEPROVIDER()->getSvcMgr();
    REGISTER_FDSP_MSG_HANDLER_GENERIC(svcMgr->getSvcRequestHandler(), fpi::FileTransferMsg ,handleTransferRequest);
    REGISTER_FDSP_MSG_HANDLER_GENERIC(svcMgr->getSvcRequestHandler(), fpi::FileTransferVerifyMsg ,handleVerifyRequest);
}

/**
 *  On the sending side..
 */
bool FileTransferService::send(const fpi::SvcUuid &svcId,
                          const std::string& srcFile,
                          const std::string& destFile,
                          OnTransferCallback cb,
                          bool fDeleteFileAfterTransfer) {
    GLOGDEBUG << "here";
    // first check if the file is already there.
    fds_uint64_t hashCode = getHashCode(svcId, srcFile);

    GLOGDEBUG << " send: " << srcFile << " as [" << destFile <<"] to " << svcId.svc_uuid;

    if (transferMap.end() != transferMap.find(hashCode)) {
        GLOGWARN << "transfer info already exists [svc: " << svcId.svc_uuid << ":" << srcFile << "]";
        return false;
    }

    FileTransferHandle::ptr handle(new FileTransferHandle(srcFile, destFile, svcId, this));
    handle->cb = cb;
    handle->fDeleteFileAfterTransfer = fDeleteFileAfterTransfer;
    transferMap[hashCode] = handle;
    sendVerifyRequest(handle);
    dump();
    return true;
}

std::string FileTransferService::getFullPath(const std::string& filename) {
    return destDir + filename;
}

bool FileTransferService::sendNextChunk(FileTransferHandle::ptr handle) {
    GLOGDEBUG << "before send : " << handle;
    dump();
    fpi::FileTransferMsgPtr msg(new fpi::FileTransferMsg());

    if (!handle->hasMoreData()) {
        GLOGDEBUG << "no more data to send : " << handle;
        return true;
    }
    msg->filename = handle->destFile;
    if (!handle->getNextChunk(msg->data)) {
        GLOGWARN << "unable to send more data : " << handle;
        return false;
    }
    msg->offset = handle->lastOffset;

    // send the message
    auto request =  svcMgr->getSvcRequestMgr()->newEPSvcRequest(handle->svcId);

    request->setPayload(FDSP_MSG_TYPEID(fpi::FileTransferMsg), msg);
    request->onResponseCb(RESPONSE_MSG_HANDLER(FileTransferService::handleTransferResponse, handle));
    request->invoke();
    GLOGDEBUG << "after send : " << handle;
    return true;
}

void FileTransferService::handleTransferResponse(FileTransferHandle::ptr handle,
                                                 EPSvcRequest* request,
                                                 const Error& error,
                                                 SHPTR<std::string> payload) {
    GLOGDEBUG << handle;

    if (!error.ok()) {
        GLOGERROR << "error on transfer : " << error << ":" << handle;
        // TODO(prem): what to do here ??
        handle->done(error);
        return;
    }

    if (handle->hasMoreData()) {
        sendNextChunk(handle);
    } else {
        sendVerifyRequest(handle);
    }
}

void FileTransferService::sendVerifyRequest(FileTransferHandle::ptr handle) {
    GLOGDEBUG << handle;
    fpi::FileTransferVerifyMsgPtr msg(new fpi::FileTransferVerifyMsg());

    if (handle->hasMoreData()) {
        GLOGWARN << "sending verify before all data transferred : " << handle;
    }
    msg->filename = handle->destFile;
    msg->checksum = handle->getCheckSum();

    // send the message
    auto request =  svcMgr->getSvcRequestMgr()->newEPSvcRequest(handle->svcId);
    request->setPayload(FDSP_MSG_TYPEID(fpi::FileTransferVerifyMsg), msg);
    request->onResponseCb(RESPONSE_MSG_HANDLER(FileTransferService::handleVerifyResponse, handle));
    request->invoke();
}

void FileTransferService::handleVerifyResponse(FileTransferHandle::ptr handle,
                                               EPSvcRequest* request,
                                               const Error& error,
                                               SHPTR<std::string> payload) {
    GLOGDEBUG << error << " : " << handle;
    if (!handle->hasStarted()) {
        // this is the first verify
        if (error.ok()) {
            // check sum matched so no need to transfer the file.
            LOGDEBUG << "dest has same file - no need to transfer : " << handle;
        } else if (error == ERR_CHECKSUM_MISMATCH || error == ERR_FILE_DOES_NOT_EXIST) {
            sendNextChunk(handle);
            return;
        }
    } else {
        // this is the last verify
        if (error.ok()) {
            LOGDEBUG << "file transfer successful : " << handle;
            // successful transfer
        } else if (error == ERR_CHECKSUM_MISMATCH) {
            // some problem in transfer
            LOGERROR << error << " : " << handle;
        }
    }

    handle->done(error);
}

/************************************************************************************************
 *  On the receiving side..
 */

void FileTransferService::handleTransferRequest(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                                SHPTR<fpi::FileTransferMsg>& message) {
    GLOGDEBUG << "here";
    std::string destFile = destDir + message->filename;
    std::fstream ofs;
    Error err;
    std::ios_base::openmode mode =  std::ios::out;

    if (util::fileExists(destFile)) {
        mode |= std::ios::in;
    }

    GLOGDEBUG << "will write data @ offset:" << message->offset << " to " << destFile;
    // GLOGDEBUG << message->data;
    ofs.open (destFile.c_str(), mode);
    if (!ofs) {
        GLOGERROR << "unable to open file [" << destFile << "]";
        err = ERR_DISK_WRITE_FAILED;

    } else {
        ofs.seekp (message->offset, ofs.beg);
        LOGDEBUG << "will write [" <<message->data.size() << "] bytes @ " << ofs.tellp();
        ofs << message->data;
        if (ofs.bad()) {
            GLOGERROR << "error on write to file [" << destFile << "]";
            err = ERR_DISK_WRITE_FAILED;
        }
    }
    ofs.close();
    sendTransferResponse(asyncHdr, message, err);
}

void FileTransferService::sendTransferResponse(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                               SHPTR<fpi::FileTransferMsg>& message,
                                               const Error &e) {
    GLOGDEBUG << "here";
    dump();
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    svcMgr->getSvcRequestHandler()->sendAsyncResp(*asyncHdr, fpi::FileTransferMsgTypeId, fpi::EmptyMsg());
}

void FileTransferService::handleVerifyRequest(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                              SHPTR<fpi::FileTransferVerifyMsg>& message) {
    GLOGDEBUG << "here";
    Error err;
    std::string destFile = destDir + message->filename;
    if (!util::fileExists(destFile)) {
        err = ERR_FILE_DOES_NOT_EXIST;
    } else {
        std::string chksum = util::getFileChecksum(destFile);        
        if (chksum != message->checksum) {
            GLOGERROR << "file checksum mismatch [orig:" << message->checksum
                      << " new:" << chksum << "]"
                      << " file:" << destFile;
            err = ERR_CHECKSUM_MISMATCH;
        }
    }

    sendVerifyResponse(asyncHdr, message, err);
}

void FileTransferService::sendVerifyResponse(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                             SHPTR<fpi::FileTransferVerifyMsg>& message,
                                             const Error &e) {
    GLOGDEBUG << "here";
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    svcMgr->getSvcRequestHandler()->sendAsyncResp(*asyncHdr, fpi::FileTransferVerifyMsgTypeId, fpi::EmptyMsg());
}

void FileTransferService::done(fds_uint64_t hashCode, const Error& error) {
    FileTransferHandle::ptr handle = get(hashCode);
    if (!handle) {
        LOGERROR << "done received for handle that does not exist : [" <<  hashCode << "]";
        return;
    }

    if (handle->cb) {
        handle->cb(handle->svcId, handle->srcFile, error);
    } else {
        LOGWARN << "no cb for : " << handle;
    }

    // now remove the handle from the map
    transferMap.erase(hashCode);
}

void FileTransferService::dump() {
    GLOGDEBUG << (void*)this << "[File Transfer infos] : " << transferMap.size();
    for (const auto & item : transferMap) {
        GLOGDEBUG << item.first << " : " << item.second;
    }
}


std::ostream& operator <<(std::ostream& os, const fds::net::FileTransferHandle& handle) {
    os << "["
       << "src:" << handle.srcFile << " "
       << "sz:" << handle.fileSize << " "
       << "rd:" << handle.totalDataRead << " "
       << "offset:" << handle.lastOffset << " "
       << "dest:" << handle.destFile
       << "]";
    return os;
}

std::ostream& operator <<(std::ostream& os, const fds::net::FileTransferHandle::ptr& handle) {
    if (handle != NULL) {
        os << (*handle);
    } else {
        os << "[FileTransferHandle is NULL]";
    }
    return os;
}

}  // namespace net
}  // namespace fds
