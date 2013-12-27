#ifndef _COMM_UTILS_H_
#define _COMM_UTILS_H_

#include "gen/putget.pb.h"
#include "include/zhelpers.hpp"
#include <string>
extern "C" {
#include <czmq.h>
}

namespace Fds {

template<class T>
int zmq_msg_to_fds_proto(const zmq::message_t &message,
    T &fds_proto)
{
  // todo: zero copy
  int size = message.size();
  std::string data(static_cast<const char*>(message.data()), size);
  fds_proto.ParseFromString(data);
  return 0;
}

template<class T>
int fds_proto_to_zmq_msg(const T &fds_proto, zmq::message_t &message)
{
  // todo: zero copy
  std::string proto_str;
  if (!fds_proto.SerializeToString(&proto_str)) {
    // todo: return error code for not serializing
    return -1;
  }

  message.rebuild(proto_str.size());
  memcpy (message.data(), proto_str.data(), proto_str.size());
  return 0;
}

class ZMQAsyncEndPoint {
public:
  ZMQAsyncEndPoint(zmq::context_t &zmq_ctx, const std::string &addr)
  : zmq_ctx_(zmq_ctx),
    _socket(zmq_ctx, ZMQ_DEALER)
  {
    //s_set_id (_socket);
    std::string id("MyId");
    _socket.setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
    // todo: check the return value?
    _socket.connect(addr.c_str());
  }

  ~ZMQAsyncEndPoint() {
    // todo: close the socket
  }

  zmq::socket_t& get_socket() { return _socket; }

  int send_msg(Fds::FdsReq &msg) {
    // todo: optimize for doing zero copy. This may require whole bunch of
    // refactoring.
    std::string msg_str;
    if (!msg.SerializeToString(&msg_str)) {
      // todo: return error code for not serializing
      return -1;
    }
    // todo: check for return value
    s_send(_socket, msg_str);

    return 0;
  }
private:
  zmq::context_t &zmq_ctx_;
  zmq::socket_t _socket;
};

} // namespace fds
#endif
