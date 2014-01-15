/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <fds_process.h>
#include <NetSession.h>

class exampleDataPathRespIf : public FDS_ProtocolInterface::FDSP_DataPathRespIf {
  public:
    exampleDataPathRespIf() {
    }
    ~exampleDataPathRespIf() {
    }

    void GetObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_GetObjType& get_obj_req) {
    }
    void GetObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_GetObjType>& get_obj_req) {
    }
    void PutObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_PutObjType& put_obj_req) {
    }
    void PutObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_PutObjType>& put_obj_req) {
    }
    void DeleteObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DeleteObjType& del_obj_req) {
    }
    void DeleteObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_DeleteObjType>& del_obj_req) {
    }
    void OffsetWriteObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_OffsetWriteObjType& offset_write_obj_req) {
    }
    void OffsetWriteObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_OffsetWriteObjType>&
        offset_write_obj_req) {
    }
    void RedirReadObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_RedirReadObjType& redir_write_obj_req) {
    }
    void RedirReadObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_RedirReadObjType>&
        redir_write_obj_req) {
    }
};

int main(int argc, char *argv[]) {
    fds::Module *smVec[] = {
        nullptr
    };

    /*
     * TODO: Make this a smart pointer. The setupClientSession interface
     * needs to change to support this.
     */
    exampleDataPathRespIf *edpri = new exampleDataPathRespIf();

    boost::shared_ptr<netSessionTbl> nst =
            boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_HVISOR));

    std::string remoteIp  = "127.0.0.1";
    fds_uint32_t numChannels = 1;
    netSession *exampleSession = nst->startSession(remoteIp,
                                                   8888,
                                                   FDSP_STOR_MGR,
                                                   numChannels,
                                                   reinterpret_cast<void*>(edpri));

    boost::shared_ptr<FDSP_DataPathReqClient> client =
            dynamic_cast<netDataPathClientSession *>(exampleSession)->getClient();  // NOLINT

    return 0;
}
