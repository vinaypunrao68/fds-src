#include "DataMgr.h"

#include <iostream>

// namespace fds {

DataMgr *dataMgr;

int DataMgr::run(int argc, char* argv[]) {
  std::string endPointStr;
  if(argc > 1)
  {
    cerr << appName() << ": too many arguments" << endl;
    return EXIT_FAILURE;
  }
  
  callbackOnInterrupt();
  string udpEndPoint = "udp -p 9600";
  string tcpEndPoint = "tcp -p 6901";
  
  Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("DataMgrSvr", tcpEndPoint);

  dataMgrSrv = new DataMgrI();
  adapter->add(dataMgrSrv, communicator()->stringToIdentity("DataMgrSvr"));

  dataMgrCli = new DataMgrClientI();
  adapter->add(dataMgrCli, communicator()->stringToIdentity("DataMgrCli"));

  /*
   * Init the dm
   */
  init();
  
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

DataMgrI::DataMgrI() {
}

DataMgrI::~DataMgrI() {
}

DataMgrClientI::DataMgrClientI() {
}

DataMgrClientI::~DataMgrClientI() {
}

void DataMgrI::PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) {
}

void DataMgrI::GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&) {
}

void DataMgrI::UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&) {
  std::cout << "Update Catalog Object!!!!" << std::endl
            << "Volume offset: " << update_catalog->volume_offset << std::endl
            << std::hex
            << "Object ID high: " << (fds_uint32_t)update_catalog->data_obj_id.hash_high << std::endl
            << "Object ID low: " << (fds_uint32_t)update_catalog->data_obj_id.hash_low << std::endl
            << "Transaction ID: " << std::dec << update_catalog->dm_transaction_id << std::endl
            << "Operation: " << update_catalog->dm_operation << std::endl;

  dm_open_txn_req_t dm_req;
  dm_req.vvc_vol_id = update_catalog->volume_offset;
  dm_req.vvc_obj_id.obj_id.hash_high =
      update_catalog->data_obj_id.hash_high;
  dm_req.vvc_obj_id.obj_id.hash_low =
      update_catalog->data_obj_id.hash_low;
  dm_req.vvc_blk_id = update_catalog->data_obj_id.hash_low;
  dm_req.txn_id = 0;
  dm_req.vvc_update_time = 0;

  handle_open_txn_req(NULL, (dm_req_t *)&dm_req);
  handle_commit_txn_req(NULL, (dm_req_t *)&dm_req);

  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;
  dataMgr->swapMgrId(msg_hdr);
  dataMgr->dataMgrCli->UpdateCatalogObjectResp(msg_hdr, update_catalog);
}

void DataMgrI::OffsetWriteObject(const FDSP_MsgHdrTypePrx& msg_hdr, const FDSP_OffsetWriteObjTypePrx& offset_write_obj, const Ice::Current&) {
}

void DataMgrI::RedirReadObject(const FDSP_MsgHdrTypePrx &msg_hdr, const FDSP_RedirReadObjTypePrx& redir_read_obj, const Ice::Current&) {
}

void
DataMgrClientI::PutObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) 
{
}

void
DataMgrClientI::GetObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr,
                              const FDSP_GetObjTypePtr& get_obj,
                              const Ice::Current&) {
}

void
DataMgrClientI::UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, 
                                        const FDSP_UpdateCatalogTypePtr& update_catalog ,
                                        const Ice::Current&) 
{
}

void
DataMgrClientI::OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& msg_hdr,
                                      const FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                                      const Ice::Current&)
{
}

void
DataMgrClientI::RedirReadObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr,
                                    const FDSP_RedirReadObjTypePtr& redir_read_obj,
                                    const Ice::Current&) {
}

int main(int argc, char *argv[]) {

  dataMgr = new DataMgr();
  
  dataMgr->main(argc, argv, "config.server");
}

// }  // namespace fds
