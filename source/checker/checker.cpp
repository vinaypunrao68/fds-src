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

#include <net/net_utils.h>
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

void
BaseChecker::mod_load_from_config()
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
/*
    plf_om_ip_str    = conf.get_abs<std::string>("fds.plat.om_ip");
    plf_om_ctrl_port = conf.get_abs<int>("fds.plat.om_port");
    plf_my_ctrl_port = conf.get_abs<int>("fds.checker.control_port");
    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = plf_my_ip;
*/
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
    DatapathRespImpl() {
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

class MetaDatapathRespImpl : public FDS_ProtocolInterface::FDSP_MetaDataPathRespIf {
 public:
    MetaDatapathRespImpl() {
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
        std::cout << "digest found : " << cat_obj_req->obj_list[0].data_obj_id.digest
                  << std::endl;
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

 private:
    // XXX: list of blobs in volume goes here
    BlobDescriptorListType *resp_vector;
    FDSP_BlobDigestType *resp_digest;
    FDSP_BlobObjectList *resp_obj_list;
    concurrency::TaskStatus *get_resp_monitor_;
    friend class VolBasedChecker;
};

DirBasedChecker::DirBasedChecker(const FdsConfigAccessor &conf_helper)
    : BaseChecker("DirBasedChecker"),
      conf_helper_(conf_helper),
      get_resp_monitor_(0),
      dp_resp_handler_(new DatapathRespImpl()),
      md_resp_handler_(new MetaDatapathRespImpl())
{
}

DirBasedChecker::DirBasedChecker()
    : BaseChecker("DirBasedChecker"),
      get_resp_monitor_(0),
      dp_resp_handler_(new DatapathRespImpl()),
      md_resp_handler_(new MetaDatapathRespImpl())
{
}

DirBasedChecker::~DirBasedChecker()
{
}

void DirBasedChecker::mod_startup()
{
    /* set up nst */
    std::string myIp = net::get_local_ip(conf_helper_.get_abs<std::string>("fds.nic_if"));
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

netMetaDataPathClientSession *
DirBasedChecker::get_metadatapath_session(const NodeUuid& node_id)
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


            // TODO(bao): need stub filed in
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
            // TODO(bao): stub
        if (md_list.size() > 1) {
            compare_against(md_list[0], md_list);
        }
    }
    LOGNORMAL << "Checker run done!" << std::endl;
    LOGNORMAL << cntrs_.toString();
}

VolBasedChecker::VolBasedChecker() : DirBasedChecker()
{
}

VolBasedChecker::~VolBasedChecker()
{
}

void VolBasedChecker::get_blob_id(const NodeUuid &dm_node_id,
                                  const fds_volid_t vol_id,
                                  const std::string vol_name,
                                  const std::string blob_name,
                                  netMetaDataPathClientSession *md_session)
{
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr
      (new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_req
      (new FDS_ProtocolInterface::FDSP_QueryCatalogType);

    msg_hdr->glob_volume_id = vol_id;
    msg_hdr->msg_code      = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_REQ;
    msg_hdr->src_id        = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->src_node_name = "myipaddr";
    msg_hdr->dst_id        = FDS_ProtocolInterface::FDSP_DATA_MGR;
    msg_hdr->result        = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code      = ERR_OK;
    msg_hdr->req_cookie    = 0;
    msg_hdr->session_uuid  = md_session->getSessionId();

    query_req->blob_name             = blob_name;
    // We don't currently specify a version
    query_req->blob_version          = blob_version_invalid;
    query_req->dm_transaction_id     = 1;
    query_req->dm_operation          = FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
    query_req->obj_list.clear();
    query_req->meta_list.clear();


    /* call to DM for it */
    boost::shared_ptr<FDSP_MetaDataPathReqClient> client = md_session->getClient();
    fds_assert(client != NULL);

    /* response buffer */
    get_resp_monitor_.reset(1);
    // XXX: md_resp_handler_->blob_list = NULL;
    md_resp_handler_->get_resp_monitor_ = &get_resp_monitor_;
    md_resp_handler_->resp_digest = &resp_digest;
    md_resp_handler_->resp_obj_list = &resp_obj_list;

    client->QueryCatalogObject(msg_hdr, query_req);

    /* wait for blob list response */
    get_resp_monitor_.await();

    // md_resp_handler_->blob_list = nullptr;
    md_resp_handler_->get_resp_monitor_ = nullptr;
    md_resp_handler_->resp_digest = nullptr;
    md_resp_handler_->resp_obj_list = nullptr;

    std::cout << "recv get blob id response" << std::endl;

    ObjectID objId;
    objId.SetId(resp_obj_list[0].data_obj_id.digest.c_str(),
                resp_obj_list[0].data_obj_id.digest.length());

    /* Issue gets from SMs */
    auto nodes = clust_comm_mgr_->get_dlt()->getNodes(objId);
    for (uint32_t i = 0; i < nodes->getLength(); i++) {
        std::string ret_obj_data;
        bool ret = get_object((*nodes)[i], objId, ret_obj_data);
        fds_verify(ret == true);
        // XXX: verify fds_verify(ret_obj_data == obj_data);

        cntrs_.obj_cnt.incr();

        LOGDEBUG << "oid: " << objId<< " check passed against: "
                << std::hex << ((*nodes)[i]).uuid_get_val();
    }
    LOGNORMAL << "Checker run done!" << std::endl;
}

void VolBasedChecker::list_bucket(const NodeUuid &dm_node_id,
                                  const fds_volid_t vol_id,
                                  const std::string vol_name)
{
    // TODO(xxx): return blob_list;
    return;
}

void VolBasedChecker::run_checker()
{
    fds_volid_t         vol_id      = 0;
    int                 n_nodes     = 0;
    fds_uint64_t        dm_node_ids[2];  // FDS_REPLICATION_FACTOR
    NodeUuid            node_id;
    std::string         vol_name    = "volume";
    OMgrClient         *om_client   = NULL;

    memset(dm_node_ids, 0x00, sizeof(fds_uint64_t) * 2);

    // XXX: for now, create the volume and search for the volume id, hardcode it here.
    // fds_assert(vol_id || !"How to get volume ID from hash");
    vol_id = 3482843330729363692LL;

    // get blobs
    om_client = clust_comm_mgr_->get_om_client();
    fds_assert(om_client);
#if 0
    // XXX: why I did not recv a DMT update? OMgrClient::recvDMTUpdate() was never called.
    om_client->getDMTNodesForVolume(vol_id, dm_node_ids, &n_nodes);
    fds_assert(n_nodes > 0);
    node_id = dm_node_ids[0];
#else
    // type 1
    node_id.uuid_set_val(2116172010113598034ULL);

    /* in DM
    node type 1 is DM.
    Domain register uuid
    svc uuid 155a4b311068ffa3, node type 1
    */
#endif


#if 0
    list_bucket(node_id, vol_id, vol_name);
    // for each of the blob, go to OM to get SM responsible for it

    // then get the object data from sm
    auto nodes = clust_comm_mgr_->get_dlt()->getNodes(objId);
    for (uint32_t i = 0; i < nodes->getLength(); i++) {
        std::string ret_obj_data;
        bool ret = true;  // XXX: todo
        fds_verify(ret == true);
        fds_verify(ret_obj_data == obj_data);

        cntrs_.obj_cnt.incr();

        LOGDEBUG << "oid: " << objId<< " check passed against: "
                << std::hex << ((*nodes)[i]).uuid_get_val();
    }
    LOGNORMAL << "Checker run done!" << std::endl;
#endif
}

FdsCheckerProc::FdsCheckerProc(int argc, char *argv[],
               const std::string &config_file,
               const std::string &base_path,
               Platform *platform, Module **mod_vec)
    : PlatformProcess(argc, argv, config_file, base_path, platform, mod_vec)
{
    reinterpret_cast<DirBasedChecker *>(platform)->set_cfg_accessor(conf_helper_);
    checker_ = reinterpret_cast<DirBasedChecker *>(platform);
    g_fdslog->setSeverityFilter(
        fds_log::getLevelFromName(conf_helper_.get<std::string>("log_severity")));
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
    checker_->run_checker();
    g_cntrs_mgr->export_to_ostream(std::cout);
    std::cout << "Checker completed\n";
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
#if 1
    DirBasedChecker *plt = new DirBasedChecker;
#else
    VolBasedChecker *plt = new VolBasedChecker;
#endif
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
