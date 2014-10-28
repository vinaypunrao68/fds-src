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

service TestAMSvcSync {
    binary getBlob(1:string domainName, 2:string volumeName, 3:string blobName, 4:i32 length, 5:i32 offset);
}
