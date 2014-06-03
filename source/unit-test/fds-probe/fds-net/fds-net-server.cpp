/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <string>
#include <fds_process.h>
#include <NetSession.h>

netSession *exampleSession;  // Single global session for now
boost::shared_ptr<FDSP_DataPathRespClient> respClient;

namespace FDS_ProtocolInterface {
class exampleDataPathReqIf :
        virtual public FDSP_DataPathReqIf {
  public:
    exampleDataPathReqIf() {
    }
    ~exampleDataPathReqIf() {
    }
    void GetObject(const FDSP_MsgHdrType& fdsp_msg,
                   const FDSP_GetObjType& get_obj_req) {
        std::cout << "Got a non-shared-ptr get object message" << std::endl;
    }
    void GetObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, // NOLINT
                   boost::shared_ptr<FDSP_GetObjType>& get_obj_req) {
        std::cout << "Got a get object message" << std::endl;
        respClient =
                dynamic_cast<netDataPathServerSession *>(exampleSession)  // NOLINT
                ->getRespClient(fdsp_msg->src_node_name);
        boost::shared_ptr<FDSP_MsgHdrType> respMsg =
            boost::shared_ptr<FDSP_MsgHdrType>(new FDSP_MsgHdrType());
        boost::shared_ptr<FDSP_GetObjType> respGet =
            boost::shared_ptr<FDSP_GetObjType>(new FDSP_GetObjType());
        respClient->GetObjectResp(respMsg, respGet);
    }
    void PutObject(const FDSP_MsgHdrType& fdsp_msg,
                   const FDSP_PutObjType& put_obj_req) {
        std::cout << "Got a non-shared-ptr put object message" << std::endl;
    }
    void PutObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                   boost::shared_ptr<FDSP_PutObjType>& put_obj_req) {
        std::cout << "Got a put object message" << std::endl;
        std::string ipStr = netSession::ipAddr2String(fdsp_msg->src_ip_lo_addr);
        respClient =
                dynamic_cast<netDataPathServerSession *>(exampleSession)-> // NOLINT
                getRespClient(fdsp_msg->src_node_name);
        boost::shared_ptr<FDSP_MsgHdrType> respMsg =
            boost::shared_ptr<FDSP_MsgHdrType>(new FDSP_MsgHdrType());
        boost::shared_ptr<FDSP_PutObjType> respPut =
            boost::shared_ptr<FDSP_PutObjType>(new FDSP_PutObjType());
        respClient->PutObjectResp(respMsg, respPut);
    }
    void DeleteObject(const FDSP_MsgHdrType& fdsp_msg,
                      const FDSP_DeleteObjType& del_obj_req) {
    }
    void DeleteObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                      boost::shared_ptr<FDSP_DeleteObjType>& del_obj_req) {
    }
    void OffsetWriteObject(const FDSP_MsgHdrType& fdsp_msg,
                           const FDSP_OffsetWriteObjType& offset_write_obj_req) {
    }
    void OffsetWriteObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                           boost::shared_ptr<FDSP_OffsetWriteObjType>&
                           offset_write_obj_req) {
    }
    void RedirReadObject(const FDSP_MsgHdrType& fdsp_msg,
                         const FDSP_RedirReadObjType& redir_write_obj_req) {
    }
    void RedirReadObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
                         boost::shared_ptr<FDSP_RedirReadObjType>& redir_write_obj_req) {
    }
    void GetObjectMetadata(const FDSP_GetObjMetadataReq& metadata_req) {
    }
    void GetObjectMetadata(boost::shared_ptr<FDSP_GetObjMetadataReq>& metadata_req) {  // NOLINT
    }
    void GetTokenMigrationStats(FDSP_TokenMigrationStats& _return,
            const FDSP_MsgHdrType& fdsp_msg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void GetTokenMigrationStats(FDSP_TokenMigrationStats& _return,
            boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)  // NOLINT
    {
    }

};
}  // namespace FDS_ProtocolInterface

std::string getMyIp() {
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;
    std::string myIp;

    /*
     * Get the local IP of the host.
     * This is needed by the OM.
     */
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPv4
            if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
                struct sockaddr_in *tmpSinAddr = (struct sockaddr_in *)ifa->ifa_addr;
                tmpAddrPtr = &(tmpSinAddr->sin_addr);
                char addrBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
                myIp = std::string(addrBuf);
                if (myIp.find("10.1") != std::string::npos)
                    break;  /* TODO: more dynamic */
            }
        }
    }
    assert(myIp.empty() == false);
    return myIp;
}

int main(int argc, char **argv) {
    boost::shared_ptr<exampleDataPathReqIf> edpri(new exampleDataPathReqIf());

    boost::shared_ptr<netSessionTbl> nst =
            boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));

    std::string sessionName = "Example server";
    std::string myIpStr = getMyIp();
    int myIpInt = netSession::ipString2Addr(myIpStr);
    std::string myNodeName = "Example SM";
    exampleSession = nst->createServerSession<netDataPathServerSession>(myIpInt,
                                              8888,
                                              myNodeName,
                                              FDSP_STOR_HVISOR,
                                              edpri);

    nst->listenServer(exampleSession);

    return 0;
}
