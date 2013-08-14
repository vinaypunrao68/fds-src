#include "DataMgr.h"

#include <iostream>

namespace fds {

DataMgr *dataMgr;

Error DataMgr::_process_open(fds_uint32_t vol_offset,
                             fds_uint32_t trans_id,
                             const ObjectID& oid) {
  Error err;

  /*
   * Check the map to see if we have know about the volume.
   * TODO: We should not implicitly create the volume here!
   * We should only be creating the volume on OM volume create
   * requests.
   * TODO: Just hack the name as the offset for now.
   */
  if (vol_meta_map.count(vol_offset) == 0) {
    vol_meta_map[vol_offset] = VolumeMeta(std::to_string(vol_offset),
                                          vol_offset);
  }

  VolumeMeta &vol_meta = vol_meta_map[vol_offset];
  err = vol_meta.OpenTransaction(vol_offset, oid);  

  return err;
}

Error DataMgr::_process_close() {
  Error err(ERR_OK);

  /*
   * Here we should be updating the TVC to reflect the commit
   * and adding back to the VVC. The TVC can be an in-memory
   * update.
   * For now, we don't need to do anything because it was put
   * into the VVC on open.
   */

  return err;
}

DataMgr::DataMgr() {
  dm_log = new fds_log("dm", "logs");

  

  FDS_PLOG(dm_log) << "Constructing the Data Manager";
}

DataMgr::~DataMgr() {

  FDS_PLOG(dm_log) << "Destructing the Data Manager";

  delete dm_log;
}

int DataMgr::run(int argc, char* argv[]) {
  std::string endPointStr;
  if(argc > 1)
  {
    std::cerr << appName() << ": too many arguments" << std::endl;
    return EXIT_FAILURE;
  }
  
  callbackOnInterrupt();
  std::string udpEndPoint = "udp -p 9600";
  std::string tcpEndPoint = "tcp -p 6901";
  
  Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("DataMgrSvr", tcpEndPoint);

  // reqHandleSrv = new ReqHandler(communicator(), this);
  reqHandleSrv = new ReqHandler(communicator());
  adapter->add(reqHandleSrv, communicator()->stringToIdentity("DataMgrSvr"));

  // respHandleCli = new RespHandler();
  // adapter->add(respHandleCli, communicator()->stringToIdentity("DataMgrCli"));

  adapter->activate();
  
  communicator()->waitForShutdown();
  return EXIT_SUCCESS;
}

void DataMgr::swapMgrId(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg) {
 FDSP_MgrIdType temp_id;
 temp_id = fdsp_msg->dst_id;
 fdsp_msg->dst_id = fdsp_msg->src_id;
 fdsp_msg->src_id = temp_id;
}

fds_log* DataMgr::GetLog() {
  return dm_log;
}

void
DataMgr::interruptCallback(int)
{
  delete dm_log;
  communicator()->shutdown();
}

DataMgr::ReqHandler::ReqHandler(const Ice::CommunicatorPtr& communicator) {
}

DataMgr::ReqHandler::~ReqHandler() {
}

DataMgr::RespHandler::RespHandler() {
}

DataMgr::RespHandler::~RespHandler() {
}

void DataMgr::ReqHandler::PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) {
}

void DataMgr::ReqHandler::GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&) {
}

void DataMgr::ReqHandler::UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&) {
  Error err(ERR_OK);

  ObjectID oid(update_catalog->data_obj_id.hash_high,
               update_catalog->data_obj_id.hash_low);

  FDS_PLOG(dataMgr->GetLog()) << "Processing update catalog request with "
                              << "volume offset: " << update_catalog->volume_offset
                              << ", Obj ID " << oid
                              << ", Trans ID " << update_catalog->dm_transaction_id
                              << ", OP ID " << update_catalog->dm_operation;
  
  /*
   * For now, just treat this as an open
   */
  err = dataMgr->_process_open(update_catalog->volume_offset,
                               update_catalog->dm_transaction_id,
                               oid);
  

  /*
  dm_open_txn_req_t dm_req;
  dm_req.vvc_vol_id = update_catalog->volume_offset;
  dm_req.vvc_obj_id.SetId(update_catalog->data_obj_id.hash_high,
                          update_catalog->data_obj_id.hash_low);
  dm_req.vvc_blk_id = update_catalog->data_obj_id.hash_low;
  dm_req.txn_id = 0;
  dm_req.vvc_update_time = 0;

  handle_open_txn_req(NULL, (dm_req_t *)&dm_req);
  handle_commit_txn_req(NULL, (dm_req_t *)&dm_req);

  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;
  dataMgr->swapMgrId(msg_hdr);
  dataMgr->dataMgrCli->UpdateCatalogObjectResp(msg_hdr, update_catalog);
  */
}

void DataMgr::ReqHandler::OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&) {
}

void DataMgr::ReqHandler::RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                                          const FDSP_RedirReadObjTypePtr& redir_read_obj,
                                          const Ice::Current&) {
}

void DataMgr::ReqHandler::AssociateRespCallback(const Ice::Identity& ident,
                                                const Ice::Current& current) {
  std::cout << "Associating response Callback client to DataMgr`"
            << _communicator->identityToString(ident) << "'"<< std::endl;

  dataMgr->respHandleCli = FDSP_DataPathRespPrx::uncheckedCast(current.con->createProxy(ident));
}

void
DataMgr::RespHandler::PutObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) 
{
}

void
DataMgr::RespHandler::GetObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr,
                              const FDSP_GetObjTypePtr& get_obj,
                              const Ice::Current&) {
}

void
DataMgr::RespHandler::UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, 
                                        const FDSP_UpdateCatalogTypePtr& update_catalog ,
                                        const Ice::Current&)
{
}

void
DataMgr::RespHandler::OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& msg_hdr,
                                      const FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                                      const Ice::Current&)
{
}

void
DataMgr::RespHandler::RedirReadObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr,
                                    const FDSP_RedirReadObjTypePtr& redir_read_obj,
                                    const Ice::Current&) {
}

}  // namespace fds

int main(int argc, char *argv[]) {

  fds::dataMgr = new fds::DataMgr();
  
  fds::dataMgr->main(argc, argv, "config.server");
}
