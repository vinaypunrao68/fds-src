#ifndef __Net_Session_h__
#define __Net_Session_h__
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fdsp/FDSP_MetaDataPathReq.h>
#include <fdsp/FDSP_MetaDataPathResp.h>
#include <concurrency/Mutex.h>
#include <fds_types.h>


using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

typedef void  (*sessionErrorCallback)(string ip_addr, FDSP_MgrIdType mgrId, int channel, int errno, std::string errMsg); 
class netSession {
public:
        netSession();
	netSession(const std::string& ip_addr_str_arg, int port, 
                         std::string node_name,
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id);
	netSession(int  ip_addr_int, int port,
                         std::string node_name,
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id);
        static string ipAddr2String(int ipaddr);
        static int ipString2Addr(string ipaddr_str);
        void setSessionErrHandler(sessionErrorCallback cback);
        

        ~netSession();
    	void     endSession();

	int 		node_index;
	int 		channel_number;
	short   	proto_type;
	std::string 	ip_addr_str;
        int 		ip_addr;
	fds_uint32_t    port_num;
        FDS_ProtocolInterface::FDSP_MgrIdType mgrId;
        sessionErrorCallback cback;

        boost::shared_ptr<TTransport> socket;
        boost::shared_ptr<TTransport> transport;
        boost::shared_ptr<TProtocol> protocol;

        //Req and Resp handlers
	FDSP_DataPathReq  fdspDPAPI;
        FDSP_DataPathResp fdspDataPathResp;
	FDSP_MetaDataPathReq  fdspMetaDataReq;
        FDSP_MetaDataPathResp fdspMetaDataPathResp;
	FDSP_ControlPathReq  fdspControlPathReq;
        FDSP_ControlPathResp fdspControlPathResp;
	FDSP_ConfigPathReq  fdspConfigPathReq;
        FDSP_ConfigPathResp fdspConfigPathResp;

        netSession& operator=(const netSession& rhs) {
          if (this != &rhs) {
            node_index = rhs.node_index;
            proto_type = rhs.proto_type;
            ip_addr_str = rhs.ip_addr_str;
            ip_addr    = rhs.ip_addr;
            mgrId      = rhs.mgrId;
            fdspDPAPI  = rhs.fdspDPAPI;
            fdspDataPathResp = rhs.fdspDataPathResp;
          }
          return *this;
        }
};

inline std::ostream& operator<<(std::ostream& out, const netSession& ep) {
  out << "Network endpoint ";
  if (ep.mgrId == FDSP_DATA_MGR) {
    out << "DM";
  } else if (ep.mgrId == FDSP_STOR_MGR) {
    out << "SM";
  } else if (ep.mgrId == FDSP_ORCH_MGR) {
    out << "OM";
  } else {
    assert(ep.mgrId == FDSP_STOR_HVISOR);
    out << "SH";
  }
  out << " with IP " << ep.ip_addr
      << " and port " << ep.port_num;
  return out;
}

class netSessionTbl {
public :
    netSessionTbl(std::string src_node_name, int src_ipaddr, int port);
    netSessionTbl() {
        sessionTblMutex = new fds_mutex("RPC Tbl mutex"); 
     };
    ~netSessionTbl();
    
    int src_ipaddr;
    std::string src_node_name;
    int port;
    int role; // Server or Client side binding

    std::unordered_map<std::string dest_name_key, netSession *>    sessionTbl;
    fds_mutex   *sessionTblMutex;

// Client Procedures
    netSession*       startSession(int  dst_ipaddr, int port, FDSP_MgrIdType mgr_id, int num_channels);

    netSession*       startSession(std::string dst_node_name, int port, FDSP_MgrIdType mgr_id, int num_channels);

    void 	      endSession(int  dst_ip_addr, FDSP_MgrIdType);

    void 	      endSession(string  dst_node_name, FDSP_MgrIdType);

    void 	      endSession(netSession *);

// client side getSession
    netSession*       getSession(int dst_ip_addr, FDSP_MgrIdType mgrId);

    netSession*       getSession(string dst_node_name, FDSP_MgrIdType mgrId);

// Server side getSession
    netSession*       getSession(std::string dst_node_name, FDSP_MgrIdType mgrId);

    netSession*       getSession(std::string dst_node_name, FDSP_MgrIdType mgrId);
   
// Server Procedures
    // Create a new server session
    netSession*       createServerSession(int  local_ipaddr, int port, std::string local_node_name, FDSP_MgrIdType mgr_id);

    netSession*       createServerSession(std::string  local_node_name, int port, FDSP_MgrIdType mgr_id);

    void              listenSession(netSession* server_session); // Blocking call equivalent to .run or .serve

    void              endServerSession(netSession *server_session );
};

#endif
