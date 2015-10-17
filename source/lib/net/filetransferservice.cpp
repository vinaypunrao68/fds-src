/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>

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
                                       const fpi::SvcUuid& svcId) : srcFile(srcFile),
                                                                    destFile(destFile),
                                                                    svcId(svcId) {
    is.open(srcFile.c_str(), std::ifstream::binary);
    if (!is) {
        GLOGERROR << "unable to open file [" << srcFile << "]";
        return;
    }
    is.seekg (0, is.end);
    fileSize = is.tellg();
    is.seekg (0, is.beg);
}

bool FileTransferHandle::hasMoreData() const {
    return totalDataRead < fileSize;
}

bool FileTransferHandle::getNextChunk(std::string& data) {    
    data.reserve(FT_CHUNKSIZE);
    data.clear();
    char * buffer = new char [FT_CHUNKSIZE];
    bool fReturn = true;
    lastOffset = totalDataRead;    
    is.read(buffer, FT_CHUNKSIZE);
    if (is.good()) {
        data.assign(buffer, is.gcount());
        totalDataRead += is.gcount();
    } else {
        fReturn = false;
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

FileTransferService::FileTransferService(const std::string& destDir) {
    this->destDir = destDir;
    FdsRootDir::fds_mkdir(destDir.c_str());

    // register handlers
    REGISTER_GLOBAL_MSG_HANDLER(fpi::FileTransferMsg ,handleTransferRequest);
    REGISTER_GLOBAL_MSG_HANDLER(fpi::FileTransferVerifyMsg ,handleVerifyRequest);
}

/**
 *  On the sending side..
 */
bool FileTransferService::send(const fpi::SvcUuid &svcId,
                          const std::string& srcFile,
                          const std::string& destFile,
                          OnTransferCallback cb,
                          bool fDeleteFileAfterTransfer) {
    // first check if the file is already there.
    fds_uint64_t hashCode = getHashCode(svcId, srcFile);

    if (transferMap.end() != transferMap.find(hashCode)) {
        LOGWARN << "transfer info already exists [svc: " << svcId.svc_uuid << ":" << srcFile << "]";
        return false;
    }

    FileTransferHandle::ptr handle(new FileTransferHandle(srcFile, destFile, svcId));
    handle->cb = cb;
    handle->fDeleteFileAfterTransfer = fDeleteFileAfterTransfer;
    transferMap[hashCode] = handle;
    if (handle->hasMoreData()) {
        sendNextChunk(handle);
    } else {
        LOGWARN << "No data to send from file : " << srcFile;
        transferMap.erase(hashCode);
        return false;
    }
    return true;
}

bool FileTransferService::sendNextChunk(FileTransferHandle::ptr handle) {
    fpi::FileTransferMsgPtr msg(new fpi::FileTransferMsg());

    if (!handle->hasMoreData()) {
        LOGDEBUG << "no more data to send : [" << handle->srcFile << "]";
        return true;
    }
    msg->filename = handle->destFile;
    if (!handle->getNextChunk(msg->data)) {
        LOGWARN << "unable to send more data : [" << handle->srcFile << "]";
        return false;
    }
    msg->offset = handle->lastOffset;

    // send the message
    auto request =  gSvcRequestPool->newEPSvcRequest(handle->svcId);

    request->setPayload(FDSP_MSG_TYPEID(fpi::FileTransferMsg), msg);
    request->onResponseCb(RESPONSE_MSG_HANDLER(FileTransferService::handleTransferResponse, handle));
    request->invoke();
    return true;
}

void FileTransferService::handleTransferResponse(FileTransferHandle::ptr handle,
                                                 EPSvcRequest* request,
                                                 const Error& error,
                                                 SHPTR<std::string> payload) {

    if (!error.ok()) {
        LOGERROR << "error on transfer : " << error << ":" << handle;
        // TODO(prem): what to do here ??
        return;
    }

    if (handle->hasMoreData()) {
        sendNextChunk(handle);
    } else {
        sendVerifyRequest(handle);
    }    
}

void FileTransferService::sendVerifyRequest(FileTransferHandle::ptr handle) {
    fpi::FileTransferVerifyMsgPtr msg(new fpi::FileTransferVerifyMsg());

    if (handle->hasMoreData()) {
        LOGWARN << "sending verify before all data transferred : [" << handle->srcFile << "]";
    }
    msg->filename = handle->destFile;
    msg->checksum = util::getFileChecksum(handle->srcFile);

    // send the message
    auto request =  gSvcRequestPool->newEPSvcRequest(handle->svcId);
    request->setPayload(FDSP_MSG_TYPEID(fpi::FileTransferVerifyMsg), msg);
    request->onResponseCb(RESPONSE_MSG_HANDLER(FileTransferService::handleVerifyResponse, handle));
    request->invoke();
}

void FileTransferService::handleVerifyResponse(FileTransferHandle::ptr handle,
                                               EPSvcRequest* request,
                                               const Error& error,
                                               SHPTR<std::string> payload) {

    if (!error.ok()) {
        LOGERROR << "checksum did not match : " << error << ":" << handle;
        // TODO(prem): what to do here ??
        return;
    }

    // to callback
    handle->cb(handle->svcId, handle->srcFile, error);
}

/************************************************************************************************
 *  On the receiving side..
 */

void FileTransferService::handleTransferRequest(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                                SHPTR<fpi::FileTransferMsg>& message) {
    std::string destFile = destDir + message->filename;
    std::ofstream ofs;
    Error err;
    ofs.open (destFile.c_str(), std::ofstream::out | std::ofstream::binary);
    if (!ofs) {
        LOGERROR << "unable to open file [" << destFile << "]";
        err = ERR_DISK_WRITE_FAILED;
        
    } else {
        ofs.seekp (message->offset, ofs.beg);
        ofs << message->data;
        if (!ofs.good()) {
            LOGERROR << "error on write to file [" << destFile << "]";
            err = ERR_DISK_WRITE_FAILED;
        }
    }
    ofs.close();
    sendTransferResponse(asyncHdr, message, err);
}

void FileTransferService::sendTransferResponse(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                               SHPTR<fpi::FileTransferMsg>& message,
                                               const Error &e) {
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    SEND_ASYNC_RESP(*asyncHdr, fpi::FileTransferMsgTypeId, fpi::EmptyMsg());
}

void FileTransferService::handleVerifyRequest(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                              SHPTR<fpi::FileTransferVerifyMsg>& message) {

    std::string destFile = destDir + message->filename;
    std::string chksum = util::getFileChecksum(destFile);
    Error err;
    if (chksum != message->checksum) {
        LOGERROR << "file checksum mismatch [orig:" << message->checksum 
                 << " new:" << chksum << "]"
                 << " file:" << destFile;
        err = ERR_CHECKSUM_MISMATCH;
    }

    sendVerifyResponse(asyncHdr, message, err);
}

void FileTransferService::sendVerifyResponse(SHPTR<fpi::AsyncHdr>& asyncHdr,
                                             SHPTR<fpi::FileTransferVerifyMsg>& message,
                                             const Error &e) {

    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    SEND_ASYNC_RESP(*asyncHdr, fpi::FileTransferVerifyMsgTypeId, fpi::EmptyMsg());
}


std::ostream& operator <<(std::ostream& os, const fds::net::FileTransferHandle& handle) {
    os << "["
       << "src:" << handle.srcFile << ","
       << "sz:" << handle.fileSize << ","
       << "rd:" << handle.totalDataRead << ","
       << "offset:" << handle.lastOffset
       << "]";
    return os;
}

}  // namespace net
}  // namespace fds

