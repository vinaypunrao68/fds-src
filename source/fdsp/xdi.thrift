/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

include "common.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

struct VolumeStatus {
    1: required i64 blobCount,
    2: required i64 currentUsageInBytes,
}

struct RequestId {
    1: required string id;
}

struct ObjectOffset {
    1: required i64 value,
}

struct TxDescriptor {
    1: required i64 txId
}

service XdiService { 
    void attachVolume(1: string domainName, 2:string volumeName)
        throws (1: common.ApiException e),

    list<common.BlobDescriptor> volumeContents(1:string domainName, 2:string volumeName, 3:i32 count,
                                               4:i64 offset, 5:string pattern, 6:common.BlobListOrder orderBy, 7:bool descending)
        throws (1: common.ApiException e),

    common.BlobDescriptor statBlob(1: string domainName, 2:string volumeName, 3:string blobName)
        throws (1: common.ApiException e),

    TxDescriptor startBlobTx(1:string domainName, 2:string volumeName, 3:string blobName, 4:i32 blobMode)
        throws (1: common.ApiException e),

    void commitBlobTx(1:string domainName, 2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc)
        throws (1: common.ApiException e),

    void abortBlobTx(1:string domainName, 2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc)
        throws (1: common.ApiException e),

    binary getBlob(1:string domainName, 2:string volumeName, 3:string blobName, 4:i32 length, 5:ObjectOffset offset)
        throws (1: common.ApiException e),

    void updateMetadata(1:string domainName, 2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc, 5:map<string, string> metadata)
        throws (1: common.ApiException e),

    void updateBlob(1:string domainName,
                    2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc, 5:binary bytes, 6:i32 length, 7:ObjectOffset objectOffset, 9:bool isLast) throws (1: common.ApiException e),

    void updateBlobOnce(1:string domainName,
                        2:string volumeName, 3:string blobName, 4:i32 blobMode, 5:binary bytes, 6:i32 length, 7:ObjectOffset objectOffset, 8:map<string, string> metadata) throws (1: common.ApiException e),

    void deleteBlob(1:string domainName, 2:string volumeName, 3:string blobName)
        throws (1: common.ApiException e)

        VolumeStatus volumeStatus(1:string domainName, 2:string volumeName)
        throws (1: common.ApiException e),
}

service AsyncXdiServiceRequest {
    oneway void handshakeStart(1:RequestId requestId 2:i32 portNumber),

    oneway void attachVolume(1:RequestId requestId, 2: string domainName,
                             3:string volumeName),

    oneway void volumeContents(1:RequestId requestId, 2:string domainName,
                               3:string volumeName, 4:i32 count, 5:i64 offset, 6:string pattern,
                               7:common.BlobListOrder orderBy, 8:bool descending),

    oneway void setVolumeMetadata(1:RequestId requestId, 2:string domainName,
                                  3:string volumeName, 4:map<string, string> metadata),

    oneway void statBlob(1:RequestId requestId, 2:string domainName,
                         3:string volumeName, 4:string blobName),

    oneway void startBlobTx(1:RequestId requestId, 2:string domainName,
                            3:string volumeName, 4:string blobName, 5:i32 blobMode),

    oneway void commitBlobTx(1:RequestId requestId, 2:string domainName,
                             3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc),

    oneway void abortBlobTx(1:RequestId requestId, 2:string domainName,
                            3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc),

    oneway void getBlob(1:RequestId requestId, 2:string domainName,
                        3:string volumeName, 4:string blobName, 5:i32 length, 6:ObjectOffset offset),

    oneway void getBlobWithMeta(1:RequestId requestId, 2:string domainName,
                                3:string volumeName, 4:string blobName, 5:i32 length, 6:ObjectOffset offset),

    oneway void updateMetadata(1:RequestId requestId, 2:string domainName,
                               3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc, 6:map<string, string> metadata),

    oneway void updateBlob(1:RequestId requestId, 2:string domainName,
                           3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc, 6:binary bytes, 
                           7:i32 length, 8:ObjectOffset objectOffset, 9:bool isLast),

    oneway void updateBlobOnce(1:RequestId requestId, 2:string domainName,
                               3:string volumeName, 4:string blobName, 5:i32 blobMode, 6:binary bytes, 
                               7:i32 length, 8:ObjectOffset objectOffset, 9:map<string, string> metadata),

    oneway void deleteBlob(1:RequestId requestId, 2:string domainName,
                           3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc),

    oneway void volumeStatus(1:RequestId requestId, 2:string domainName, 3:string volumeName)
}

service AsyncXdiServiceResponse {
    oneway void handshakeComplete(1:RequestId requestId),

    oneway void attachVolumeResponse(1:RequestId requestId),

    oneway void volumeContents(1:RequestId requestId, 2:list<common.BlobDescriptor> response),

    oneway void setVolumeMetadataResponse(1:RequestId requestId),

    oneway void statBlobResponse(1:RequestId requestId, 2:common.BlobDescriptor response),

    oneway void startBlobTxResponse(1:RequestId requestId, 2:TxDescriptor response),

    oneway void commitBlobTxResponse(1:RequestId requestId),

    oneway void abortBlobTxResponse(1:RequestId requestId),

    oneway void getBlobResponse(1:RequestId requestId, 2:binary response),

    oneway void getBlobWithMetaResponse(1:RequestId requestId, 2:binary data, 3:common.BlobDescriptor blobDesc),

    oneway void updateMetadataResponse(1:RequestId requestId),

    oneway void updateBlobResponse(1:RequestId requestId),

    oneway void updateBlobOnceResponse(1:RequestId requestId),

    oneway void deleteBlobResponse(1:RequestId requestId),

    oneway void volumeStatus(1:RequestId requestId, 2:VolumeStatus response),

    oneway void completeExceptionally(1:RequestId requestId, 2:common.ErrorCode errorCode, 3:string message)
}
