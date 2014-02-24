/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fstream>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/progress.hpp>

#include <fds_volume.h>
#include <fdsp/FDSP_types.h>
#include <hash/MurmurHash3.h>
#include <checker.h>

namespace fs = boost::filesystem;

namespace fds {

class DatapathRespImpl : public FDS_ProtocolInterface::FDSP_DataPathRespIf {
 public:
    explicit DatapathRespImpl() {
        get_obj_buf_ = nullptr;
        get_resp_monitor_ = nullptr;
    }

    ~DatapathRespImpl() {
    }

    void GetObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const FDS_ProtocolInterface::FDSP_GetObjType& get_obj_req) {
    }

    void PutObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const FDS_ProtocolInterface::FDSP_PutObjType& put_obj_req) {
    }

    void DeleteObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const FDS_ProtocolInterface::FDSP_DeleteObjType& del_obj_req) {
    }


    void OffsetWriteObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const FDS_ProtocolInterface::FDSP_OffsetWriteObjType& offset_write_obj_req) {
    }


    void RedirReadObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const FDS_ProtocolInterface::FDSP_RedirReadObjType& redir_write_obj_req) {
    }


    void PutObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,  // NOLINT
            FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_req) {  // NOLINT
    }
    void DeleteObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,  // NOLINT
            FDS_ProtocolInterface::FDSP_DeleteObjTypePtr& del_req) {  // NOLINT
    }

    void OffsetWriteObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&  // NOLINT
            fdsp_msg,
            FDS_ProtocolInterface::FDSP_OffsetWriteObjTypePtr& offset_write_obj_req) {  // NOLINT
    }
    void RedirReadObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,  // NOLINT
            FDS_ProtocolInterface::FDSP_RedirReadObjTypePtr&redir_write_obj_req) {  // NOLINT
    }

    void GetObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,  // NOLINT
            FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_req) {  // NOLINT
         fds_verify(msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);
         *get_obj_buf_ = get_req->data_obj;
         get_resp_monitor_->done();
    }

 private:
    std::string *get_obj_buf_;
    concurrency::TaskStatus *get_resp_monitor_;
    friend class FdsDataChecker;
};

FdsDataChecker::FdsDataChecker(int argc, char *argv[],
               const std::string &config_file,
               const std::string &base_path)
    : FdsProcess(argc, argv, config_file, base_path),
      get_resp_monitor_(0),
      dp_resp_handler_(new DatapathRespImpl())
{
}

FdsDataChecker::~FdsDataChecker()
{
    delete clust_comm_mgr_->get_om_client();
}

void FdsDataChecker::setup(int argc, char *argv[],
                       fds::Module **mod_vec)
{
    FdsProcess::setup(argc, argv, mod_vec);


    /* set up nst */
    std::string myIp = netSession::getLocalIp();
    netSessionTblPtr nst(new netSessionTbl(FDSP_STOR_HVISOR));

    /* set up om client */
    OMgrClient *om_client;
    om_client = new OMgrClient(FDSP_STOR_HVISOR,
                              conf_helper_.get<std::string>("om_ip"),
                              conf_helper_.get<int>("om_port"),
                              myIp,
                              0,
                              "localhost-checker",
                              GetLog(),
                              nst,
                              0);

    om_client->initialize();

    FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr dInfo;
    dInfo.reset(new FDSP_AnnounceDiskCapability());
    dInfo->disk_iops_max =  10000; /* avarage IOPS */
    dInfo->disk_iops_min =  100; /* avarage IOPS */
    dInfo->disk_capacity = 100;  /* size in GB */
    dInfo->disk_latency_max = 100; /* in milli second */
    dInfo->disk_latency_min = 10; /* in milli second */
    dInfo->ssd_iops_max =  100000; /* avarage IOPS */
    dInfo->ssd_iops_min =  1000; /* avarage IOPS */
    dInfo->ssd_capacity = 100;  /* size in GB */
    dInfo->ssd_latency_max = 100; /* in milli second */
    dInfo->ssd_latency_min = 3; /* in milli second */
    dInfo->disk_type =  FDS_DISK_SATA;
    om_client->startAcceptingControlMessages(conf_helper_.get<int>("control_port"));
    om_client->registerNodeWithOM(dInfo);

    /* Set up cluster comm manager */
    clust_comm_mgr_.reset(new ClusterCommMgr(om_client, nst));

    /* wait until we have dlt */
    while (clust_comm_mgr_->get_dlt() == nullptr) {sleep(1);}
}

netDataPathClientSession*
FdsDataChecker::get_datapath_session(const NodeUuid& node_id)
{
    unsigned int ip;
    fds_uint32_t port;
    int state;
    netDataPathClientSession *dp_session;
    netSessionTblPtr nst = clust_comm_mgr_->get_nst();

    clust_comm_mgr_->get_om_client()->getNodeInfo(
            node_id.uuid_get_val(), &ip, &port, &state);

    netSession* session = nst->getSession(ip, FDSP_STOR_MGR);
    if (session == nullptr) {
      dp_session = nst->startSession<netDataPathClientSession>(static_cast<int>(ip),
              port,
              FDSP_STOR_MGR,
              1, /* number of channels */
              dp_resp_handler_);
    } else {
        dp_session = static_cast<netDataPathClientSession*>(session);
    }

    fds_verify(dp_session != nullptr);
    return dp_session;
}

bool FdsDataChecker::read_file(const std::string &filename, std::string &content)
{
    std::ifstream ifs(filename);
    content.assign((std::istreambuf_iterator<char>(ifs)),
            (std::istreambuf_iterator<char>()));
    return true;
}

bool FdsDataChecker::get_object(const NodeUuid& node_id,
        const ObjectID &oid, std::string &content)
{
    auto dp_session = get_datapath_session(node_id);

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
            new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req(
            new FDS_ProtocolInterface::FDSP_GetObjType);
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    msg_hdr->session_uuid = dp_session->getSessionId();
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->glob_volume_id = FdsSysTaskQueueId;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;

    get_req->data_obj_id.hash_high = oid.GetHigh();
    get_req->data_obj_id.hash_low  = oid.GetLow();
    get_req->data_obj_len          = 0;

    /* response buffer */
    get_resp_monitor_.reset(1);
    dp_resp_handler_->get_obj_buf_= &content;
    dp_resp_handler_->get_resp_monitor_ = &get_resp_monitor_;

    dp_session->getClient()->GetObject(msg_hdr, get_req);

    /* we will block until we get response */
    get_resp_monitor_.await();

    dp_resp_handler_->get_obj_buf_ = nullptr;
    dp_resp_handler_->get_resp_monitor_ = nullptr;

    return true;
}

void FdsDataChecker::run()
{
    fs::path full_path = fs::system_complete(conf_helper_.get<std::string>("path"));

    fds_verify(fs::exists(full_path));

    fds_verify(fs::is_directory(full_path));

    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr( full_path );
            dir_itr != end_iter; ++dir_itr ) {
        if (fs::is_directory(dir_itr->status())) {
            continue;
        }
        /* Read the file data */
        std::string obj_data;
        ObjectID oid;
        bool ret = read_file(dir_itr->path().native(), obj_data);
        fds_verify(ret == true);
        MurmurHash3_x64_128(obj_data.c_str(),
                obj_data.size(),
                0,
                &oid);

        /* Issue gets from SMs */
        auto nodes = clust_comm_mgr_->get_dlt()->getNodes(oid);
        for (uint32_t i = 0; i < nodes->getLength(); i++) {
            std::string ret_obj_data;
            bool ret = get_object((*nodes)[i], oid, ret_obj_data);
            fds_verify(ret == true);
            fds_verify(ret_obj_data == obj_data);

            LOGDEBUG << "oid: " << oid << " check passed against: "
                    << std::hex << ((*nodes)[i]).uuid_get_val();
        }
    }
}

}  // namespace fds

int main(int argc, char *argv[]) {
    fds::FdsDataChecker *checker = new fds::FdsDataChecker(argc, argv,
            "checker.conf", "fds.checker.");
    checker->setup(argc, argv, nullptr);
    checker->run();

    delete checker;
}

