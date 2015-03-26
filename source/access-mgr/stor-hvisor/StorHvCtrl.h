/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_STORHVISOR_STORHVCTRL_H_
#define SOURCE_ACCESS_MGR_STORHVISOR_STORHVCTRL_H_

#include <string>

#include "fds_error.h"
#include "fds_volume.h"
#include "FdsRandom.h"
#include "NetSession.h"
#include "util/Log.h"
#include "concurrency/RwLock.h"

#include "AmCounters.h"


namespace fds
{
    class AmProcessor;
    class AmRequest;
    class Callback;
    class OMgrClient;
    class SysParams;
    class StorHvQosCtrl;
}  // namespace fds

namespace fds_pi = FDS_ProtocolInterface;

namespace FDS_ProtocolInterface
{
    class FDSP_AnnounceDiskCapability;
    class FDSP_VolumeDescType;
}  // namespace FDS_ProtocolInterface

template<class T> using b_sp = boost::shared_ptr<T>;

class StorHvCtrl : public fds::HasLogger {
public:
    /*
     * Defines specific test modes used to
     * construct the object.
     */
    typedef enum {
        DATA_MGR_TEST, /* Only communicate with DMs */
        STOR_MGR_TEST, /* Only communicate with SMs */
        TEST_BOTH,     /* Communication with DMs and SMs */
        NORMAL,        /* Normal, non-test mode */
        MAX
    } sh_comm_modes;

    StorHvCtrl(int argc, char *argv[], fds::SysParams *params);
    StorHvCtrl(int argc, char *argv[], fds::SysParams *params,
               sh_comm_modes _mode);
    StorHvCtrl(int argc,
               char *argv[],
               fds::SysParams *params,
               sh_comm_modes _mode,
               fds_uint32_t sm_port_num,
               fds_uint32_t dm_port_num);
    ~StorHvCtrl();

    std::shared_ptr<fds::StorHvQosCtrl>     qos_ctrl; // Qos Controller object
    fds::OMgrClient*        om_client;

    /// Toggle AM standalone mode for testing
    fds_bool_t standalone;

    /// Processor layer module
    std::unique_ptr<fds::AmProcessor> amProcessor;

    fds::RandNumGenerator::ptr randNumGen;

    fds::Error sendTestBucketToOM(const std::string& bucket_name,
                             const std::string& access_key_id = "",
                             const std::string& secret_access_key = "");

    void attachVolume(fds::AmRequest *amReq);
    void enqueueAttachReq(const std::string& volumeName,
                          b_sp<fds::Callback> cb);
    void enqueueBlobReq(fds::AmRequest *blobReq);

    fds::SysParams* getSysParams();
    void StartOmClient();
    sh_comm_modes GetRunTimeMode() { return mode; }

    inline AMCounters& getCounters()
    { return counters_; }

    /**
     * Add blob request to wait queue -- those are blobs that
     * are waiting for OM to attach buckets to AM; once
     * vol table receives vol attach event, it will move
     * all requests waiting in the queue for that bucket to
     * appropriate qos queue
     */
    void addBlobToWaitQueue(const std::string& bucket_name,
                            AmRequest* blob_req);
    void moveWaitBlobsToQosQueue(fds_volid_t vol_uuid,
                                 const std::string& vol_name,
                                 Error error);

private:
    fds::SysParams *sysParams;
    sh_comm_modes mode;

    /// Toggles the local volume catalog cache
    fds_bool_t disableVcc;
    /** Counters */
    AMCounters counters_;

    /**
     * list of blobs that are waiting for OM to attach appropriate
     * bucket to AM if it exists/ or return 'does not exist error
     */
    typedef std::vector<AmRequest*> bucket_wait_vec_t;
    typedef std::map<std::string, bucket_wait_vec_t> wait_blobs_t;
    typedef std::map<std::string, bucket_wait_vec_t>::iterator wait_blobs_it_t;
    wait_blobs_t wait_blobs;
    fds_rwlock wait_rwlock;
};

extern StorHvCtrl* storHvisor;

// Static function for process IO via a threadpool
extern void processBlobReq(fds::AmRequest *amReq);


#endif  // SOURCE_ACCESS_MGR_STOR-HVISOR_STORHVCTRL_H_
