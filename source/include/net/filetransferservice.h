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

#define FT_CHUNKSIZE 2*MB

namespace fds { 
class EPSvcRequest;
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
                       const fpi::SvcUuid& svcId);

    bool hasMoreData() const;
    bool getNextChunk(std::string& data);
        
    ~FileTransferHandle();

    const std::string& srcFile;
    const std::string& destFile;
    const fpi::SvcUuid& svcId;
    OnTransferCallback cb;
    fds_uint64_t getHashCode() const;
  protected:
        
    std::ifstream is;
    bool fDeleteFileAfterTransfer = false;
    fds_uint64_t fileSize = 0;
    fds_uint64_t totalDataRead = 0;
    fds_int64_t lastOffset = -1;
    friend struct FileTransferService;
    friend std::ostream& operator <<(std::ostream& os, const FileTransferHandle& handle);
};

struct FileTransferService : HasLogger {
    explicit FileTransferService(const std::string& destDir);
    bool send(const fpi::SvcUuid &svcId,
              const std::string& srcFile,
              const std::string& destFile,
              OnTransferCallback cb,
              bool fDeleteFileAfterTransfer = true
              );

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
                
  protected:
    std::string destDir;
    bool exists(fds_uint64_t hashCode);
    FileTransferHandle::ptr get(fds_uint64_t hashCode);
    std::map<fds_uint64_t, SHPTR<FileTransferHandle> > transferMap;

};
std::ostream& operator <<(std::ostream& os, const FileTransferHandle& handle);
} // namespace net
} // namespace fds
#endif  // SOURCE_INCLUDE_NET_FILETRANSFERSERVICE_H_
