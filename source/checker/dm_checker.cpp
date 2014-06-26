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
#include "boost/program_options.hpp"

#include <fds_volume.h>
#include <fdsp/FDSP_types.h>
#include <hash/MurmurHash3.h>
#include <dm_checker.h>
#include <FdsCrypto.h>
#include <ObjectId.h>
#include <am-platform.h>
#include <stdlib.h>

namespace fs = boost::filesystem;

namespace fds {

void BaseChecker::log_corruption(const std::string& info) {
    if (panic_on_corruption_) {
        fds_verify(!"Encountered corruption");
    }
    // TODO(Rao:) Log corruption
}

void
BaseChecker::mod_load_from_config()
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    plf_om_ip_str    = conf.get_abs<std::string>("fds.plat.om_ip");
    plf_om_ctrl_port = conf.get_abs<int>("fds.plat.om_port");
    plf_my_ctrl_port = conf.get_abs<int>("fds.checker.control_port");
    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = plf_my_ip;
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
    friend class LevelDBChecker;
};

class MetaDatapathRespImpl : public FDS_ProtocolInterface::FDSP_MetaDataPathRespIf {
 public:
    explicit MetaDatapathRespImpl() {
        // XXX: initialize blob list
        resp_vector = nullptr;
        resp_digest = nullptr;
        get_resp_monitor_ = nullptr;
    }
    ~MetaDatapathRespImpl() {
    }

    void UpdateCatalogObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                 const FDSP_UpdateCatalogType& cat_obj_req) {
        fds_assert(0);
    }

    void UpdateCatalogObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                                 boost::shared_ptr<FDSP_UpdateCatalogType>& cat_obj_req) {
        fds_assert(0);
    }

    void QueryCatalogObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                const FDSP_QueryCatalogType& cat_obj_req) {
        fds_assert(0);
    }

    void QueryCatalogObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                                boost::shared_ptr<FDSP_QueryCatalogType>& cat_obj_req) {
        fds_verify(fdsp_msg->result == FDS_ProtocolInterface::FDSP_ERR_OK ||
                   !"blob name not exist on DM");
        fds_assert(resp_digest != nullptr);
        *resp_digest = cat_obj_req->digest;
        fds_assert(resp_obj_list);

        FDS_ProtocolInterface::FDSP_BlobObjectInfo& cat_obj_info =
            cat_obj_req->obj_list[0];

        ObjectID objId;
        objId.SetId((const char *)cat_obj_info.data_obj_id.digest.c_str(),
                cat_obj_info.data_obj_id.digest.length());

        std::cout << "digest found : " << objId << std::endl;

        *resp_obj_list = cat_obj_req->obj_list;
        // std::cout << "digest found : " << cat_obj_req->obj_list[0].data_obj_id.digest
        //          << std::endl;
        fds_assert(get_resp_monitor_);
        get_resp_monitor_->done();
    }

    void StartBlobTxResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg) {  // NOLINT
        fds_assert(0);
    }

    void StartBlobTxResp(const FDSP_MsgHdrType& fdsp_msg) {
        fds_assert(0);
    }

    void StatBlobResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                      boost::shared_ptr<BlobDescriptor>& blobDesc) {
        fds_assert(0);
    }

    void StatBlobResp(const FDSP_MsgHdrType& fdsp_msg,
                      const BlobDescriptor& blobDesc) {
        fds_assert(0);
    }

    void DeleteCatalogObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                 const FDSP_DeleteCatalogType& cat_obj_req) {
        fds_assert(0);
    }

    void DeleteCatalogObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                                 boost::shared_ptr<FDSP_DeleteCatalogType>& cat_obj_req) {
        fds_assert(0);
    }

    void SetBlobMetaDataResp(const FDSP_MsgHdrType& header, const std::string& blobName) {
        fds_assert(0);
    }


    void SetBlobMetaDataResp(boost::shared_ptr<FDSP_MsgHdrType>& header,
                             boost::shared_ptr<std::string>& blobName) {
        fds_assert(0);
    }

    void GetBlobMetaDataResp(const FDSP_MsgHdrType& header, const std::string& blobName,
                             const FDSP_MetaDataList& metaDataList) {
        fds_assert(0);
    }


    void GetBlobMetaDataResp(boost::shared_ptr<FDSP_MsgHdrType>& header,
                             boost::shared_ptr<std::string>& blobName,
                             boost::shared_ptr<FDSP_MetaDataList>& metaDataList) {
        fds_assert(0);
    }

    void GetVolumeMetaDataResp(const FDSP_MsgHdrType& header,
                               const FDSP_VolumeMetaData& volumeMeta) {
        fds_assert(0);
    }

    void GetVolumeMetaDataResp(boost::shared_ptr<FDSP_MsgHdrType>& header,
                               boost::shared_ptr<FDSP_VolumeMetaData>& volumeMeta) {
        fds_assert(0);
    }


    void GetVolumeBlobListResp(const FDSP_MsgHdrType& fds_msg,
                               const FDSP_GetVolumeBlobListRespType& blob_list_rsp) {
        fds_assert(0);
    }

    void GetVolumeBlobListResp(boost::shared_ptr<FDSP_MsgHdrType>& fds_msg,  // NOLINT
                       boost::shared_ptr<FDSP_GetVolumeBlobListRespType>& blob_list_rsp) {
        fds_verify(fds_msg->result == FDS_ProtocolInterface::FDSP_ERR_OK ||
                   !"volume ID not exist on DM");

        // Need to handle end_of_list and iterator_cookie
        fds_assert(resp_vector != nullptr);
        *resp_vector = blob_list_rsp->blob_info_list;
        BlobInfoListType *vec = &blob_list_rsp->blob_info_list;
        for (uint i = 0; i < vec->size(); i++) {
            // std::cout << *it << std::endl;
            FDSP_BlobInfoType &elm = vec->at(i);
            // std::cout << "found name " << elm.blob_name <<  std::endl;
            // std::cout << "found size " << elm.blob_size <<  std::endl;
        }

        get_resp_monitor_->done();
    }

 private:
    // XXX: list of blobs in volume goes here
    BlobInfoListType *resp_vector;
    FDSP_BlobDigestType *resp_digest;
    FDSP_BlobObjectList *resp_obj_list;
    concurrency::TaskStatus *get_resp_monitor_;
    friend class LevelDBChecker;
};

/**
 * LEVELDB CHECKER
 */

LevelDBChecker::LevelDBChecker(const FdsConfigAccessor &conf_helper)
    : BaseChecker("LevelDBChecker"),
      conf_helper_(conf_helper),
      get_resp_monitor_(0),
      dp_resp_handler_(new DatapathRespImpl()),
      md_resp_handler_(new MetaDatapathRespImpl())
{
}

LevelDBChecker::LevelDBChecker()
    : BaseChecker("LevelDBChecker"),
      get_resp_monitor_(0),
      dp_resp_handler_(new DatapathRespImpl()),
      md_resp_handler_(new MetaDatapathRespImpl())
{
}

LevelDBChecker::~LevelDBChecker()
{
}

void LevelDBChecker::mod_startup()
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
                              this);

    om_client->initialize();
    om_client->startAcceptingControlMessages();
    om_client->registerNodeWithOM(this);

    /* Set up cluster comm manager */
    clust_comm_mgr_.reset(new ClusterCommMgr(om_client, nst));

    /* wait until we have dlt */
    while (clust_comm_mgr_->get_dlt() == nullptr) {sleep(1);}
}

void LevelDBChecker::mod_shutdown()
{
    delete clust_comm_mgr_->get_om_client();
}

netMetaDataPathClientSession *
LevelDBChecker::get_metadatapath_session(const NodeUuid& node_id)
{
    unsigned int        ip    = 0;
    fds_uint32_t        port  = 0;
    int                 state = 0;
    netMetaDataPathClientSession *mdp_sess = NULL;
    netSessionTblPtr nst = clust_comm_mgr_->get_nst();

    fds_assert(nst != nullptr);

    clust_comm_mgr_->get_om_client()->getNodeInfo(
            node_id.uuid_get_val(), &ip, &port, &state);

    if (!nst->clientSessionExists(ip, port)) {
        mdp_sess = nst->startSession<netMetaDataPathClientSession>(static_cast<int>(ip),
                                                                   port,
                                                                   FDSP_DATA_MGR,
                                                                   1,
                                                                   md_resp_handler_);
    } else {
        mdp_sess = nst->getClientSession<netMetaDataPathClientSession>(
                            static_cast<int>(ip),
                            port);
    }

    fds_verify(mdp_sess != nullptr);
    return (mdp_sess);
}


PlatRpcReqt *LevelDBChecker::plat_creat_reqt_disp()
{
    return nullptr;
}

PlatRpcResp *LevelDBChecker::plat_creat_resp_disp()
{
    return nullptr;
}

void LevelDBChecker::list_bucket(const NodeUuid &dm_node_id,
                                  const fds_volid_t vol_id,
                                  const std::string vol_name)
{
    auto md_session = get_metadatapath_session(dm_node_id);

    /* get blobs for volume with DM */
    FDSP_MsgHdrTypePtr      msg_hdr(new FDSP_MsgHdrType);
    msg_hdr->msg_code        = FDSP_MSG_GET_VOL_BLOB_LIST_REQ;
    msg_hdr->msg_id          = 1;
    msg_hdr->major_ver       = 0xa5;
    msg_hdr->minor_ver       = 0x5a;
    msg_hdr->num_objects     = 1;
    msg_hdr->frag_len        = 0;
    msg_hdr->frag_num        = 0;
    msg_hdr->tennant_id      = 0;
    msg_hdr->local_domain_id = 0;
    msg_hdr->local_domain_id = 0;
    msg_hdr->req_cookie      = 0;
    msg_hdr->glob_volume_id  = vol_id;
    // msg_hdr->src_ip_lo_addr  = SRC_IP;
    msg_hdr->dst_id          = FDSP_DATA_MGR;
    msg_hdr->err_code        = ERR_OK;
    msg_hdr->src_port        = 0;
    // msg_hdr->src_node_name   = storHvisor->myIp;
    msg_hdr->src_node_name   = "myipaddr";
    // msg_hdr->bucket_name    = blobReq->getBlobName();
    msg_hdr->bucket_name     = vol_name;
    msg_hdr->session_uuid    = md_session->getSessionId();

    /* format request */
    FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqTypePtr get_bucket_list_req(
            new FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqType);

    boost::shared_ptr<FDSP_MetaDataPathReqClient> client = md_session->getClient();
    fds_assert(client != NULL);

    /* response buffer */
    get_resp_monitor_.reset(1);

    // XXX: md_resp_handler_->blob_list = NULL;
    md_resp_handler_->get_resp_monitor_ = &get_resp_monitor_;
    md_resp_handler_->resp_vector = &resp_vector;
    // md_resp_handler_->resp_obj_list = &resp_obj_list;

    client->GetVolumeBlobList(msg_hdr, get_bucket_list_req);

    /* wait for blob list response */
    get_resp_monitor_.await();

    // md_resp_handler_->blob_list = nullptr;
    md_resp_handler_->get_resp_monitor_ = nullptr;
    md_resp_handler_->resp_vector = nullptr;

    // std::cout << "list bucket recv a response" << std::endl;

    return;  // resp_vector;
}

FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr
LevelDBChecker::query_catalog(const NodeUuid &dm_node_id,
                              const fds_volid_t vol_id,
                              const std::string vol_name,
                              const std::string blob_name)
{
auto md_session = get_metadatapath_session(dm_node_id);

    /* get blobs for volume with DM */
    FDSP_MsgHdrTypePtr      msg_hdr(new FDSP_MsgHdrType);
    msg_hdr->msg_code        = FDSP_MSG_QUERY_CAT_OBJ_REQ;
    msg_hdr->msg_id          = 1;
    msg_hdr->major_ver       = 0xa5;
    msg_hdr->minor_ver       = 0x5a;
    msg_hdr->num_objects     = 1;
    msg_hdr->frag_len        = 0;
    msg_hdr->frag_num        = 0;
    msg_hdr->tennant_id      = 0;
    msg_hdr->local_domain_id = 0;
    msg_hdr->local_domain_id = 0;
    msg_hdr->req_cookie      = 0;
    msg_hdr->glob_volume_id  = vol_id;
    msg_hdr->dst_id          = FDSP_DATA_MGR;
    msg_hdr->err_code        = ERR_OK;
    msg_hdr->src_port        = 0;
    msg_hdr->src_node_name   = "myipaddr";
    msg_hdr->bucket_name     = vol_name;
    msg_hdr->session_uuid    = md_session->getSessionId();

    /* format request */
    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_req(
        new FDS_ProtocolInterface::FDSP_QueryCatalogType);

    boost::shared_ptr<FDSP_MetaDataPathReqClient> client = md_session->getClient();
    fds_assert(client != NULL);

    /* response buffer */
    get_resp_monitor_.reset(1);

    // XXX: md_resp_handler_->blob_list = NULL;
    md_resp_handler_->get_resp_monitor_ = &get_resp_monitor_;
    md_resp_handler_->resp_digest = &resp_digest;
    md_resp_handler_->resp_obj_list = &resp_obj_list;

    // Setup get_query_catalog_req
    query_req->blob_name = blob_name;
    query_req->blob_version = blob_version_invalid;
    query_req->dm_transaction_id = 1;
    query_req->dm_operation = FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
    query_req->obj_list.clear();
    query_req->meta_list.clear();

    client->QueryCatalogObject(msg_hdr, query_req);

    /* wait for blob list response */
    get_resp_monitor_.await();

    // md_resp_handler_->blob_list = nullptr;
    md_resp_handler_->get_resp_monitor_ = nullptr;
    md_resp_handler_->resp_vector = nullptr;

    // std::cout << "query catalog recv a response" << std::endl;

    return query_req;
}



void LevelDBChecker::run_checker(const char *volume)
{
    const char *vol_name = volume;
    std::string v_name(vol_name);

    fds::ResourceUUID uuid(fds_get_uuid64(vol_name));
    fds_volid_t volume_id = uuid.uuid_get_val();

    // Get om_client
    auto om_client = clust_comm_mgr_->get_om_client();

    std::cout << "Got volume ID: " << volume_id << std::endl;

    DmtColumnPtr col_ptr = om_client->getDMTNodesForVolume(volume_id);
    std::cout << "Got col_ptr " << col_ptr << std::endl;

    // Col 0 is master, everything should check against this
    list_bucket(col_ptr->get(0), volume_id, v_name);
    // Create copy of master blob infos
    BlobInfoListType master(resp_vector);

    // For each row in the table
    for (uint i = 1; i < col_ptr->getLength(); ++i) {
        // Get the BlobInfoVector
        list_bucket(col_ptr->get(i), volume_id, v_name);
        std::cout << "Comparing master vs " << col_ptr->get(i) << std::endl;
        // Should now have a resp_vector -> BlobInfoListType
        for (uint j = 0; j < master.size(); ++j) {
            FDSP_BlobInfoType &m_elm = master.at(j);
            FDSP_BlobInfoType &elm = resp_vector.at(j);
            // Get catalog info for this object
            // std::cout << "Getting catalog for blob " << m_elm.blob_name << std::endl;

            FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr m_query_resp =
                    query_catalog(col_ptr->get(i), volume_id, v_name, m_elm.blob_name);

            FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_resp =
                    query_catalog(col_ptr->get(i), volume_id, v_name, elm.blob_name);

            fds_assert(m_query_resp != query_resp);
            fds_assert(m_query_resp->blob_name == query_resp->blob_name);
            fds_assert(m_query_resp->blob_size == query_resp->blob_size);
            fds_assert(m_query_resp->digest == query_resp->digest);

            fds_assert(m_query_resp->obj_list.size() ==
                       query_resp->obj_list.size());

            std::cout << "Verifying objects list..." << std::endl;
            for (uint p = 0; p < m_query_resp->obj_list.size(); ++p) {
                fds_assert(m_query_resp->obj_list[p] ==
                           query_resp->obj_list[p]);
            }
        }
    }

    std::cout << "Checker run done, all tests PASSED!" << std::endl;
    LOGNORMAL << cntrs_.toString();
}

//-----------------------------------------------

FdsCheckerProc::FdsCheckerProc(int argc, char *argv[],
               const std::string &config_file,
               const std::string &base_path,
               Platform *platform, Module **mod_vec)
    : PlatformProcess(argc, argv, config_file, base_path, platform, mod_vec)
{
    reinterpret_cast<LevelDBChecker *>(platform)->set_cfg_accessor(conf_helper_);
    checker_ = reinterpret_cast<LevelDBChecker *>(platform);
    g_fdslog->setSeverityFilter(
            (fds_log::severity_level) conf_helper_.get<int>("log_severity"));


    // Get --volume from command line args
    namespace po = boost::program_options;
    po::options_description desc("Run a LevelDB Checker");

    desc.add_options()
            ("help,h", "Print this help message")
            ("volume,v", po::value<std::string>(),
             "Name of the volume to check.");

    po::variables_map vm;
    po::parsed_options parsed =
            po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(0);
    }

    if (vm.count("volume") == 0) {
        std::cout << desc << "\n";
        exit(-1);
    }

    volume = vm["volume"].as<std::string>().c_str();
}

FdsCheckerProc::~FdsCheckerProc()
{
}

void FdsCheckerProc::proc_pre_startup()
{
    checker_->mod_load_from_config();
    PlatformProcess::proc_pre_startup();

    // checker_->mod_startup();
}

int FdsCheckerProc::run()
{
    checker_->run_checker(volume);
    g_cntrs_mgr->export_to_ostream(std::cout);
    // std::cout << "Checker completed\n";
    return 0;
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
}

}  // namespace fds

int main(int argc, char *argv[]) {
    LevelDBChecker *plt = new LevelDBChecker;
    fds::Module *checker_mod_vec[] = {
            plt,
            nullptr
    };

    /* platform cannot be on the stack */
    fds::FdsCheckerProc *checker = new fds::FdsCheckerProc(argc, argv,
            "fds.checker.", "checker.log", plt, checker_mod_vec);
    checker->main();

    delete checker;

    return (0);
}
