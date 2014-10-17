include "../../fdsp/fds_service.thrift"

namespace cpp FDS_ProtocolInterface

service TestSMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void putObject(1: fds_service.AsyncHdr asyncHdr, 2: fds_service.PutObjectMsg payload);
}

service TestAMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void putObjectRsp(1: fds_service.AsyncHdr asyncHdr, 2: fds_service.PutObjectRspMsg payload);
}
