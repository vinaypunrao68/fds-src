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

#include "AmCounters.h"


namespace fds
{
    class AmCache;
    class AmDispatcher;
    class AmProcessor;
    class AmRequest;
    class AmTxManager;
    class Callback;
    class OMgrClient;
    class SysParams;
    class StorHvQosCtrl;
    class StorHvVolumeTable;
}  // namespace fds

namespace fds_pi = FDS_ProtocolInterface;

namespace FDS_ProtocolInterface
{
    class FDSP_AnnounceDiskCapability;
    class FDSP_VolumeDescType;
}  // namespace FDS_ProtocolInterface

template<class T> using b_sp = boost::shared_ptr<T>;

class StorHvDataPlacement;

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
               sh_comm_modes _mode, fds_uint32_t instanceId = 0);
    StorHvCtrl(int argc,
               char *argv[],
               fds::SysParams *params,
               sh_comm_modes _mode,
               fds_uint32_t sm_port_num,
               fds_uint32_t dm_port_num,
               fds_uint32_t instanceId = 0);
    ~StorHvCtrl();

    // Data Members
    StorHvDataPlacement*    dataPlacementTbl;
    b_sp<netSessionTbl>     rpcSessionTbl; // RPC calls Switch Table
    fds::StorHvVolumeTable* vol_table;  
    fds::StorHvQosCtrl*     qos_ctrl; // Qos Controller object
    fds::OMgrClient*        om_client;

    b_sp<fds_pi::FDSP_AnnounceDiskCapability> dInfo;

    std::string                 my_node_name;

    /// Toggle AM standalone mode for testing
    fds_bool_t toggleStandAlone;

    /// Dispatcher layer module
    b_sp<fds::AmDispatcher> amDispatcher;

    /// Processor layer module
    std::unique_ptr<fds::AmProcessor> amProcessor;

    fds::RandNumGenerator::ptr randNumGen;
    // TODO(Andrew): Move this to a unique_ptr and only into
    // AmProcessor once that's ready
    std::shared_ptr<fds::AmTxManager> amTxMgr;
    std::shared_ptr<fds::AmCache> amCache;

    fds::Error sendTestBucketToOM(const std::string& bucket_name,
                             const std::string& access_key_id = "",
                             const std::string& secret_access_key = "");

    void initVolInfo(b_sp<fds_pi::FDSP_VolumeDescType> vol_info,
                     const std::string& bucket_name);

    void attachVolume(fds::AmRequest *amReq);
    void enqueueAttachReq(const std::string& volumeName,
                          b_sp<fds::Callback> cb);
    fds::Error pushBlobReq(fds::AmRequest *blobReq);
    void enqueueBlobReq(fds::AmRequest *blobReq);

    fds::SysParams* getSysParams();
    void StartOmClient();
    sh_comm_modes GetRunTimeMode() { return mode; }

    inline AMCounters& getCounters()
    { return counters_; }

private:
    fds::SysParams *sysParams;
    sh_comm_modes mode;

    /// Toggles the local volume catalog cache
    fds_bool_t disableVcc;
    /** Counters */
    AMCounters counters_;
};

extern StorHvCtrl* storHvisor;

// Static function for process IO via a threadpool
extern void processBlobReq(fds::AmRequest *amReq);


#endif  // SOURCE_ACCESS_MGR_STOR-HVISOR_STORHVCTRL_H_
