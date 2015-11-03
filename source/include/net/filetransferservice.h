/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_FILETRANSFERSERVICE_H_
#define SOURCE_INCLUDE_NET_FILETRANSFERSERVICE_H_

#include <string>
#include <thread>

#include <fds_typedefs.h>
#include <fds_defines.h>
#include <fdsp/filetransfer_api_types.h>

namespace fds { 
class EPSvcRequest;
class SvcMgr;
namespace net {
// fwd decl
struct FileTransferService;
/**
 * func to be called after a file is transferred [un]successfully.
 */
typedef std::function<void(const fpi::SvcUuid &svcId,
                           const std::string& srcFile,
                           const Error &e)>  OnTransferCallback;

fds_uint64_t getHashCode(const fpi::SvcUuid &svcId, const std::string& srcFile);

struct FileTransferHandle {
    TYPE_SHAREDPTR(FileTransferHandle);
    FileTransferHandle(const std::string& srcFile,
                       const std::string& destFile,
                       const fpi::SvcUuid& svcId,
                       FileTransferService* ftService);

    bool hasMoreData() const;
    bool getNextChunk(std::string& data);
        
    ~FileTransferHandle();

    const std::string srcFile;
    const std::string destFile;
    const fpi::SvcUuid svcId;
    OnTransferCallback cb;
    fds_uint64_t getHashCode() const;
    const std::string& getCheckSum() const;
    void done(const Error& error);
    bool hasStarted() const;
    bool isComplete() const;
  protected:
    FileTransferService* ftService = NULL;
    std::string checkSum;
    std::ifstream is;
    bool fDeleteFileAfterTransfer = false;
    fds_uint64_t fileSize = 0;
    fds_uint64_t totalDataRead = 0;
    fds_int64_t lastOffset = -1;
    friend struct FileTransferService;
    friend std::ostream& operator <<(std::ostream& os, const FileTransferHandle& handle);
    friend std::ostream& operator <<(std::ostream& os, const FileTransferHandle::ptr& handle);
};

struct FileTransferService : HasLogger {
    TYPE_SHAREDPTR(FileTransferService);
    explicit FileTransferService(const std::string& destDir, SvcMgr* svcMgr = NULL);
    bool send(const fpi::SvcUuid &svcId,
              const std::string& srcFile,
              const std::string& destFile,
              OnTransferCallback cb,
              bool fDeleteFileAfterTransfer = true
              );

    /* get the full path of the file */
    std::string getFullPath(const std::string& filename);

    /**
     * Sender Side handlers
     */
    bool sendNextChunk(FileTransferHandle::ptr handle);
    void handleTransferResponse(FileTransferHandle::ptr handle,
                                EPSvcRequest* request,
                                const Error& error,
                                SHPTR<std::string> payload);
    void sendVerifyRequest(FileTransferHandle::ptr handle);
    void handleVerifyResponse(FileTransferHandle::ptr handle,
                              EPSvcRequest* request,
                              const Error& error,
                              SHPTR<std::string> payload);

    /**
     * Receiver Side handlers
     */
    void handleTransferRequest(SHPTR<fpi::AsyncHdr>& asyncHdr,
                               SHPTR<fpi::FileTransferMsg>& message);
    void sendTransferResponse(SHPTR<fpi::AsyncHdr>& asyncHdr,
                              SHPTR<fpi::FileTransferMsg>& message,
                              const Error &e);
    void handleVerifyRequest(SHPTR<fpi::AsyncHdr>& asyncHdr,
                             SHPTR<fpi::FileTransferVerifyMsg>& message);
    void sendVerifyResponse(SHPTR<fpi::AsyncHdr>& asyncHdr,
                            SHPTR<fpi::FileTransferVerifyMsg>& message,
                            const Error &e);

    void dump();
    void done(fds_uint64_t hashCode, const Error& error);
  protected:
    std::string destDir;
    bool exists(fds_uint64_t hashCode);
    FileTransferHandle::ptr get(fds_uint64_t hashCode);
    SvcMgr* svcMgr;
    std::map<fds_uint64_t, SHPTR<FileTransferHandle> > transferMap;

};
std::ostream& operator <<(std::ostream& os, const FileTransferHandle& handle);
std::ostream& operator <<(std::ostream& os, const FileTransferHandle::ptr& handle);
} // namespace net
} // namespace fds
#endif  // SOURCE_INCLUDE_NET_FILETRANSFERSERVICE_H_
