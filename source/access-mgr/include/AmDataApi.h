/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMDATAAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMDATAAPI_H_

#include <map>
#include <string>
#include <vector>
#include <fds_module.h>
#include <apis/AmService.h>

namespace fds {

/**
 * AM's data API that is exposed to XDI. This interface is the
 * basic data API that XDI and connectors are programmed to.
 */
class AmDataApi : public apis::AmServiceIf {
  private:
    /// Uturn test all AM service APIs
    fds_bool_t testUturnAll;
    /// Uturn test start tx API
    fds_bool_t testUturnStartTx;
    /// Uturn test update blob API
    fds_bool_t testUturnUpdateBlob;
    /// Uturn test update metadata API
    fds_bool_t testUturnUpdateMeta;
    /// Uturn test commit tx API
    fds_bool_t testUturnCommitTx;
    /// Uturn test abort tx API
    fds_bool_t testUturnAbortTx;
    /// Uturn test stat blob API
    fds_bool_t testUturnStatBlob;
    /// Uturn test get blob API
    fds_bool_t testUturnGetBlob;

    std::atomic_ullong      io_log_counter;
    fds_uint64_t            io_log_interval;

  public:
    AmDataApi();
    ~AmDataApi();
    typedef boost::shared_ptr<AmDataApi> shared_ptr;

    void attachVolume(const std::string& domainName,
                      const std::string& volumeName);
    void attachVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName);

    void volumeStatus(apis::VolumeStatus& _return,
                      const std::string& domainName,
                      const std::string& volumeName);
    void volumeStatus(apis::VolumeStatus& _return,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName);

    void volumeContents(std::vector<fpi::BlobDescriptor> & _return,
                        const std::string& domainName,
                        const std::string& volumeName,
                        const int32_t count,
                        const int64_t offset,
                        const std::string& pattern,
                        const fpi::BlobListOrder orderBy,
                        const bool descending);
    void volumeContents(std::vector<fpi::BlobDescriptor> & _return,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<int32_t>& count,
                        boost::shared_ptr<int64_t>& offset,
                        boost::shared_ptr<std::string>& pattern,
                        boost::shared_ptr<fpi::BlobListOrder>& orderBy,
                        boost::shared_ptr<bool>& descending);

    void statBlob(fpi::BlobDescriptor& _return,
                  const std::string& domainName,
                  const std::string& volumeName,
                  const std::string& blobName);
    void statBlob(fpi::BlobDescriptor& _return,
                  boost::shared_ptr<std::string>& domainName,
                  boost::shared_ptr<std::string>& volumeName,
                  boost::shared_ptr<std::string>& blobName);

    void startBlobTx(apis::TxDescriptor& _return,
                     const std::string& domainName,
                     const std::string& volumeName,
                     const std::string& blobName,
                     const fds_int32_t blobMode);

    void startBlobTx(apis::TxDescriptor& _return,
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<fds_int32_t>& blobMode);

    void commitBlobTx(const std::string& domainName,
                      const std::string& volumeName,
                      const std::string& blobName,
                      const apis::TxDescriptor& txDesc);
    void commitBlobTx(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<std::string>& blobName,
                      boost::shared_ptr<apis::TxDescriptor>& txDesc);

    void abortBlobTx(const std::string& domainName,
                     const std::string& volumeName,
                     const std::string& blobName,
                     const apis::TxDescriptor& txDesc);
    void abortBlobTx(boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<apis::TxDescriptor>& txDesc);

    void getBlob(std::string& _return,
                 const std::string& domainName,
                 const std::string& volumeName,
                 const std::string& blobName,
                 const int32_t length,
                 const apis::ObjectOffset& offset);
    void getBlob(std::string& _return,
                 boost::shared_ptr<std::string>& domainName,
                 boost::shared_ptr<std::string>& volumeName,
                 boost::shared_ptr<std::string>& blobName,
                 boost::shared_ptr<int32_t>& length,
                 boost::shared_ptr<apis::ObjectOffset>& objectOffset);

    void updateMetadata(const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const apis::TxDescriptor& txDesc,
                        const std::map<std::string, std::string> & metadata);
    void updateMetadata(boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<apis::TxDescriptor>& txDesc,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata);

    void updateBlobOnce(const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const fds_int32_t blobMode,
                        const std::string& bytes,
                        const int32_t length,
                        const apis::ObjectOffset& objectOffset,
                        const std::map<std::string, std::string>& metadata);
    void updateBlobOnce(boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<fds_int32_t>& blobMode,
                        boost::shared_ptr<std::string>& bytes,
                        boost::shared_ptr<int32_t>& length,
                        boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata);

    void updateBlob(const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName,
                    const apis::TxDescriptor& txDesc,
                    const std::string& bytes,
                    const int32_t length,
                    const apis::ObjectOffset& objectOffset,
                    const bool isLast);
    void updateBlob(boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<apis::TxDescriptor>& txDesc,
                    boost::shared_ptr<std::string>& bytes,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                    boost::shared_ptr<bool>& isLast);

    void deleteBlob(const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName);
    void deleteBlob(boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMDATAAPI_H_
