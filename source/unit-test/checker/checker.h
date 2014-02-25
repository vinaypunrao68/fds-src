/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CHECKER_H_
#define SOURCE_INCLUDE_CHECKER_H_

#include <fds_process.h>
#include <ClusterCommMgr.h>
#include <NetSession.h>
#include <concurrency/taskstatus.h>
#include <fdsp/FDSP_types.h>

namespace fds {

class DatapathRespImpl;

class FdsDataChecker : public FdsProcess {
public:
    FdsDataChecker(int argc, char *argv[],
               const std::string &config_file,
               const std::string &base_path);
    virtual ~FdsDataChecker();
    virtual void setup(int argc, char *argv[],
                       fds::Module **mod_vec) override;
    void run() override;

protected:
    bool read_file(const std::string &filename, std::string &content);
    bool get_object(const NodeUuid& node_id,
            const ObjectID &oid, std::string &content);
    netDataPathClientSession* get_datapath_session(const NodeUuid& node_id);

    ClusterCommMgrPtr clust_comm_mgr_;
    concurrency::TaskStatus get_resp_monitor_;
    boost::shared_ptr<DatapathRespImpl> dp_resp_handler_;
};

}  // namespace fds
#endif
