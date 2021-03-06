// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.


#include <stdio.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "FDSP_DataPathResp.h"
#include "FDSP_DataPathReq.h"  // As an example
#include "sh_app.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace apache::thrift::concurrency;


using boost::shared_ptr;

using namespace  ::FDS_ProtocolInterface;

class FDSP_DataPathRespHandler : virtual public FDSP_DataPathRespIf {
 public:
  FDSP_DataPathRespHandler() {
    // Your initialization goes here
  }

  void GetObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_GetObjType& get_obj_req) {
    // Your implementation goes here
    printf("GetObjectResp\n");
  }

  void PutObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_PutObjType& put_obj_req) {
    // Your implementation goes here
    printf("PutObjectResp\n");
  }

  void DeleteObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_DeleteObjType& del_obj_req) {
    // Your implementation goes here
    printf("DeleteObjectResp\n");
  }

};

void shApp_interface::app_send_data(char *buf, int len)
{
  printf(" inside shApp_interface::app_send_data \n");
}


class ResponseReceiver: public Runnable {
public:
  ResponseReceiver(boost::shared_ptr<TProtocol> prot,
                   boost::shared_ptr<FDSP_DataPathRespHandler> hdlr)
    : prot_(prot),
      processor_(new FDSP_DataPathRespProcessor(hdlr))
  {
  }

  void run() {
    /* wait for connection to be established */
    /* for now just busy wait */
    while (!prot_->getTransport()->isOpen()) {
      printf("ResponseReceiver: waiting for transport...\n");
      usleep(500);
    }

    try {
      for (;;) {
        if (!processor_->process(prot_, prot_, NULL) ||
            !prot_->getTransport()->peek()) {
          break;
        }
      }
    }
    catch (TException& tx) {
      printf("Response Receiver exception");
    }
  }


public:
  boost::shared_ptr<TProtocol> prot_;
  boost::shared_ptr<TProcessor> processor_;
};




// sh client 
int main(int argc, char **argv) {
 
  char tmpbuf[4096];

  boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  /* start thread that will receive responses to data requests */
  PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                   PosixThreadFactory::NORMAL,
                                   1,
                                   false);
  boost::shared_ptr<FDSP_DataPathRespHandler> handler(new FDSP_DataPathRespHandler());
  boost::shared_ptr<ResponseReceiver> msg_recv(new ResponseReceiver(protocol, handler));
  boost::shared_ptr<Thread> th = threadFactory.newThread(msg_recv);
  th->start();


  FDS_ProtocolInterface::FDSP_MsgHdrType fdsp_msg_hdr ;
  FDS_ProtocolInterface::FDSP_PutObjType put_obj_req ;

  FDSP_DataPathReqClient smClient(protocol);

  fdsp_msg_hdr.minor_ver = 0;
//  fdsp_msg_hdr.msg_code = FDSP_MSG_PUT_OBJ_REQ;
  fdsp_msg_hdr.msg_code = FDSP_MsgCodeType::FDSP_MSG_PUT_OBJ_REQ;
  fdsp_msg_hdr.msg_id =  1;

  fdsp_msg_hdr.major_ve = 0xa5;
  fdsp_msg_hdr.minor_ver = 0x5a;

  fdsp_msg_hdr.num_objects = 1;
  fdsp_msg_hdr.frag_len = 0;
  fdsp_msg_hdr.frag_num = 0;

  fdsp_msg_hdr.tennant_id = 0;
  fdsp_msg_hdr.local_domain_id = 0;

  fdsp_msg_hdr.src_id = FDSP_MgrIdType::FDSP_STOR_HVISOR;
  fdsp_msg_hdr.dst_id = FDSP_MgrIdType::FDSP_STOR_MGR;

//  fdsp_msg_hdr.src_node_name = this->my_node_name;

  fdsp_msg_hdr.err_code=FDSP_ErrType::FDSP_ERR_SM_NO_SPACE;
  fdsp_msg_hdr.result=FDSP_ResultType::FDSP_ERR_OK;

  put_obj_req.data_obj = std::string((const char *)tmpbuf, 4096);
  put_obj_req.data_obj_len = 4096;


  transport->open();
  smClient.PutObject(fdsp_msg_hdr, put_obj_req);
  sleep(1);
  transport->close();
  th->join();
}

