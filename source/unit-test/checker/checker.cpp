/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/progress.hpp>

#include <fds_volume.h>
#include <fdsp/FDSP_types.h>
#include <hash/MurmurHash3.h>
#include <checker.h>
#include <FdsCrypto.h>
#include <ObjectId.h>
#include <am-platform.h>

namespace fs = boost::filesystem;

namespace fds {

void BaseChecker::log_corruption(const std::string& info) {
    if (panic_on_corruption_) {
        fds_verify(!"Encountered corruption");
    }
    // TODO(Rao:) Log corruption
}

/**
 * Less than operator
 */
struct FDSP_ObjectVolumeAssociationLess {
    bool operator() (const FDSP_ObjectVolumeAssociation &a,
            const FDSP_ObjectVolumeAssociation& b) const
    {
        return a.vol_id.uuid < b.vol_id.uuid;
    }
};

/**
 * Compares golden against each element in md_list
 * @param golden
 * @param md_list
 */
void BaseChecker::compare_against(FDSP_MigrateObjectMetadata& golden,  // NOLINT
        std::vector<FDSP_MigrateObjectMetadata> md_list)
{
    std::sort(golden.associations.begin(), golden.associations.end(),
            FDSP_ObjectVolumeAssociationLess());

    for (auto entry : md_list) {
        /* We compare each entry here.  For new entries just add an
         * if statement
         */
        if (golden.token_id != entry.token_id) {
            log_corruption("golden.token_id != entry.token_id");
        }

        if (golden.object_id.digest != entry.object_id.digest) {
            log_corruption("golden.object_id.digest != entry.object_id.digest");
        }

        if (golden.obj_len != entry.obj_len) {
            log_corruption("golden.obj_len != entry.obj_len");
        }

        if (golden.born_ts != entry.born_ts) {
            log_corruption("golden.born_ts != entry.born_ts");
        }

        if (golden.modification_ts != entry.modification_ts) {
            log_corruption("golden.modification_ts != entry.modification_ts");
        }
        std::sort(entry.associations.begin(), entry.associations.end(),
                FDSP_ObjectVolumeAssociationLess());
        if (golden.associations != entry.associations) {
            log_corruption("golden.associatons != entry.associations");
        }
    }
}

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
         fds_verify(msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK ||
                    !"object ID not exist on SM");
         *get_obj_buf_ = get_req->data_obj;
         get_resp_monitor_->done();
    }

    void GetObjectMetadataResp(
                boost::shared_ptr<FDSP_GetObjMetadataResp>& metadata_resp)  // NOLINT
    {
        fds_verify(metadata_resp->header.result == FDS_ProtocolInterface::FDSP_ERR_OK ||
                            !"object ID not exist on SM");
        *get_md_buf_ = metadata_resp->meta_data;
        get_resp_monitor_->done();
    }

    void GetObjectMetadataResp(const FDSP_GetObjMetadataResp& metadata_resp) {
    }

 private:
    std::string *get_obj_buf_;
    FDSP_MigrateObjectMetadata *get_md_buf_;
    concurrency::TaskStatus *get_resp_monitor_;
    friend class DirBasedChecker;
};

DirBasedChecker::DirBasedChecker(const FdsConfigAccessor &conf_helper)
    : BaseChecker("DirBasedChecker"),
      conf_helper_(conf_helper),
      get_resp_monitor_(0),
      dp_resp_handler_(new DatapathRespImpl())
{
}

DirBasedChecker::DirBasedChecker()
    : BaseChecker("DirBasedChecker"),
      get_resp_monitor_(0),
      dp_resp_handler_(new DatapathRespImpl())
{
}

DirBasedChecker::~DirBasedChecker()
{
}

void DirBasedChecker::mod_startup()
{
    /* set up nst */
    std::string myIp = netSession::getLocalIp();
    netSessionTblPtr nst(new netSessionTbl(FDSP_STOR_HVISOR));

    /* set up om client */
    OMgrClient *om_client;
    om_client = new OMgrClient(FDSP_STOR_HVISOR,
                              conf_helper_.get<std::string>("om_ip"),
                              conf_helper_.get<int>("om_port"),
                              "localhost-checker",
                              GetLog(),
                              nst,
                              &gl_AmPlatform);

    om_client->initialize();

#if 0
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
#endif
    om_client->startAcceptingControlMessages();

    om_client->registerNodeWithOM(this);

    /* Set up cluster comm manager */
    clust_comm_mgr_.reset(new ClusterCommMgr(om_client, nst));

    /* wait until we have dlt */
    while (clust_comm_mgr_->get_dlt() == nullptr) {sleep(1);}
}

void DirBasedChecker::mod_shutdown()
{
    delete clust_comm_mgr_->get_om_client();
}

netDataPathClientSession*
DirBasedChecker::get_datapath_session(const NodeUuid& node_id)
{
    unsigned int ip;
    fds_uint32_t port;
    int state;
    netDataPathClientSession *dp_session;
    netSessionTblPtr nst = clust_comm_mgr_->get_nst();

    clust_comm_mgr_->get_om_client()->getNodeInfo(
            node_id.uuid_get_val(), &ip, &port, &state);

    if (!nst->clientSessionExists(ip, port)) {
      dp_session = nst->startSession<netDataPathClientSession>(static_cast<int>(ip),
              port,
              FDSP_STOR_MGR,
              1, /* number of channels */
              dp_resp_handler_);
    } else {
        dp_session = nst->getClientSession<netDataPathClientSession>(
                static_cast<int>(ip),
                port);
    }

    fds_verify(dp_session != nullptr);
    return dp_session;
}

bool DirBasedChecker::read_file(const std::string &filename, std::string &content)
{
    std::ifstream ifs(filename);
    content.assign((std::istreambuf_iterator<char>(ifs)),
            (std::istreambuf_iterator<char>()));
    return true;
}

bool DirBasedChecker::get_object(const NodeUuid& node_id,
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
    msg_hdr->err_code = ERR_OK;

    get_req->data_obj_id.digest = std::string((const char *)oid.GetId(),
                                              (size_t)oid.GetLen());
    get_req->data_obj_len       = 0;
    get_req->dlt_version        = 1;

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

bool DirBasedChecker::get_object_metadata(const NodeUuid& node_id,
        const ObjectID &oid, FDSP_MigrateObjectMetadata &md)
{
    auto dp_session = get_datapath_session(node_id);

    FDS_ProtocolInterface::FDSP_GetObjMetadataReqPtr get_req(
            new FDS_ProtocolInterface::FDSP_GetObjMetadataReq);
    get_req->header.msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    get_req->header.session_uuid = dp_session->getSessionId();
    get_req->header.src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    get_req->header.dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    get_req->header.result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    get_req->header.glob_volume_id = FdsSysTaskQueueId;
    get_req->header.err_code = ERR_OK;

    get_req->obj_id.digest = std::string((const char *)oid.GetId(),
                                              (size_t)oid.GetLen());


    /* response buffer */
    get_resp_monitor_.reset(1);
    dp_resp_handler_->get_md_buf_= &md;
    dp_resp_handler_->get_resp_monitor_ = &get_resp_monitor_;

    dp_session->getClient()->GetObjectMetadata(get_req);

    /* we will block until we get response */
    get_resp_monitor_.await();

    dp_resp_handler_->get_md_buf_ = nullptr;
    dp_resp_handler_->get_resp_monitor_ = nullptr;

    return true;
}

PlatRpcReqt *DirBasedChecker::plat_creat_reqt_disp()
{
    fds_assert(0);
    return nullptr;
}

PlatRpcResp *DirBasedChecker::plat_creat_resp_disp()
{
    fds_assert(0);
    return nullptr;
}

void DirBasedChecker::run_checker()
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
        bool ret = read_file(dir_itr->path().native(), obj_data);
        fds_verify(ret == true);

        /* Hash the data to object the ID */
        ObjectID objId = ObjIdGen::genObjectId(obj_data.c_str(), obj_data.size());
        std::vector<FDSP_MigrateObjectMetadata> md_list;

        /* Issue gets from SMs */
        auto nodes = clust_comm_mgr_->get_dlt()->getNodes(objId);
        for (uint32_t i = 0; i < nodes->getLength(); i++) {
            std::string ret_obj_data;
            bool ret = get_object((*nodes)[i], objId, ret_obj_data);
            fds_verify(ret == true);
            fds_verify(ret_obj_data == obj_data);

            FDSP_MigrateObjectMetadata ret_obj_md;
            ret = get_object_metadata((*nodes)[i], objId, ret_obj_md);
            fds_verify(ret == true);
            md_list.push_back(ret_obj_md);

            cntrs_.obj_cnt.incr();

            LOGDEBUG << "oid: " << objId<< " check passed against: "
                    << std::hex << ((*nodes)[i]).uuid_get_val();
        }
        /* Do metadata comparision.  Ideally we compare against expected metadata
         * Here we assume expected metadata is the first replica (this isn't right)
         * Below comparison only tells us wheter sync took place properly or not
         */
        if (md_list.size() > 1) {
            compare_against(md_list[0], md_list);
        }
    }
    LOGNORMAL << "Checker run done!" << std::endl;
}

FdsCheckerProc::FdsCheckerProc(int argc, char *argv[],
               const std::string &config_file,
               const std::string &base_path,
               Platform *platform, Module **mod_vec)
    : PlatformProcess(argc, argv, config_file, base_path, platform, mod_vec)
{
    reinterpret_cast<DirBasedChecker *>(platform)->set_cfg_accessor(conf_helper_);
    checker_.reset(reinterpret_cast<DirBasedChecker *>(platform));
    g_fdslog->setSeverityFilter(
            (fds_log::severity_level) conf_helper_.get<int>("log_severity"));
}

FdsCheckerProc::~FdsCheckerProc()
{
    checker_->mod_shutdown();
}

void FdsCheckerProc::setup()
{
    PlatformProcess::setup();

    checker_->mod_startup();
}

void FdsCheckerProc::run()
{
    checker_->run_checker();
    g_cntrs_mgr->export_to_ostream(std::cout);
}

void FdsCheckerProc::plf_load_node_data()
{
    PlatformProcess::plf_load_node_data();
    if (plf_node_data.nd_has_data == 0) {
        plf_node_data.nd_node_uuid = fds_get_uuid64(get_uuid());
        std::cout << "NO UUID, generate one " << std::hex
                  << plf_node_data.nd_node_uuid << std::endl;
    }
    plf_node_data.nd_node_uuid = fds_get_uuid64(get_uuid());
    std::cout << "UUID, generate for checker " << std::hex
              << plf_node_data.nd_node_uuid << std::endl;
    // Make up some values for now.
    //
    plf_node_data.nd_has_data    = 1;
    plf_node_data.nd_magic       = 0xcafebabe;
    plf_node_data.nd_node_number = 0;
    plf_node_data.nd_plat_port   = plf_mgr->plf_get_my_ctrl_port();
    plf_node_data.nd_om_port     = plf_mgr->plf_get_om_ctrl_port();
    plf_node_data.nd_flag_run_sm = 0;
    plf_node_data.nd_flag_run_dm = 1;
    plf_node_data.nd_flag_run_am = 2;
    plf_node_data.nd_flag_run_om = 3;

    // TODO(Vy): deal with error here.
    //
    plf_db->set(plf_db_key,
                std::string((const char *)&plf_node_data, sizeof(plf_node_data)));
}

}  // namespace fds

int main(int argc, char *argv[]) {
    DirBasedChecker *plt = new DirBasedChecker;

    /* platform cannot be on the stack */
    fds::FdsCheckerProc *checker = new fds::FdsCheckerProc(argc, argv,
            "fds.checker.", "checker.log", plt, nullptr);
    checker->setup();
    checker->run();

    delete checker;

    return (0);
}
