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
        std::cout << "Got a non-shared-ptr put object message" << std::endl;
    }
    void PutObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_PutObjType>& put_obj_req) {
        std::cout << "Got a put object message response" << std::endl;
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
    boost::shared_ptr<exampleDataPathRespIf> edpri(new exampleDataPathRespIf());
    boost::shared_ptr<netSessionTbl> nstA =
            boost::shared_ptr<netSessionTbl>(new netSessionTbl("127.0.0.1",
                                             0, 4500, 10, FDSP_STOR_HVISOR));

    std::string remoteIp  = "127.0.0.1";
    fds_uint32_t numChannels = 1;
    netDataPathClientSession *exampleSessionA =
            nstA->startSession<netDataPathClientSession>(remoteIp,
                                                         8888,
                                                         FDSP_STOR_MGR,
                                                         numChannels,
                                                         edpri);

    boost::shared_ptr<FDSP_DataPathReqClient> client =
            exampleSessionA->getClient();  // NOLINT

    boost::shared_ptr<FDSP_MsgHdrType> fdspMsg =
            boost::shared_ptr<FDSP_MsgHdrType>(new FDSP_MsgHdrType());
    fdspMsg->src_node_name = "127.0.0.1";
    boost::shared_ptr<FDSP_PutObjType> putObjReq =
            boost::shared_ptr<FDSP_PutObjType>(new FDSP_PutObjType());
    client->PutObject(fdspMsg, putObjReq);
    sleep(10);
    exampleSessionA->endSession();
    boost::shared_ptr<netSessionTbl> nstB =
            boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_HVISOR));


    return 0;
}
