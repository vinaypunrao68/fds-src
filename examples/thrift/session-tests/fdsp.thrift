#
# Testing async req/resp on the same socket
#

namespace cpp fds

struct PutObjMessage {
  1: i32 data_obj_len,
  2: string data_obj
}

service DataPathReq {
  oneway void PutObject(1: PutObjMessage req)
}

service DataPathResp {
  oneway void PutObjectResp(1: PutObjMessage req)
}

