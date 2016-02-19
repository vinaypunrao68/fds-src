/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

/**
 * As implemented currently (1/14/2016), currentUsageInBytes is
 * "logical" usage - does not account for deduping.
 */
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

service AsyncXdiServiceRequest {
    oneway void handshakeStart(1:RequestId requestId 2:i32 portNumber),

    oneway void attachVolume(1:RequestId requestId, 2: string domainName,
                             3:string volumeName, 4: common.VolumeAccessMode mode),

    oneway void volumeContents(1:RequestId requestId, 2:string domainName,
                               3:string volumeName, 4:i32 count, 5:i64 offset, 6:string pattern,
                               7:common.PatternSemantics patternSemantics,
                               8:common.BlobListOrder orderBy, 9:bool descending,
                               10:string delimiter),

    oneway void setVolumeMetadata(1:RequestId requestId, 2:string domainName,
                                  3:string volumeName, 4:map<string, string> metadata),

    oneway void getVolumeMetadata(1:RequestId requestId, 2:string domainName,
                                  3:string volumeName),

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

    oneway void renameBlob(1:RequestId requestId,
                           2:string domainName,
                           3:string volumeName,
                           4:string sourceBlobName,
                           5:string destinationBlobName),

    oneway void updateBlob(1:RequestId requestId, 2:string domainName,
                           3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc, 6:binary bytes, 
                           7:i32 length, 8:ObjectOffset objectOffset),

    oneway void updateBlobOnce(1:RequestId requestId, 2:string domainName,
                               3:string volumeName, 4:string blobName, 5:i32 blobMode, 6:binary bytes, 
                               7:i32 length, 8:ObjectOffset objectOffset, 9:map<string, string> metadata),

    oneway void deleteBlob(1:RequestId requestId, 2:string domainName,
                           3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc),

    oneway void volumeStatus(1:RequestId requestId, 2:string domainName, 3:string volumeName)
}

service AsyncXdiServiceResponse {
    oneway void handshakeComplete(1:RequestId requestId),

    oneway void attachVolumeResponse(1:RequestId requestId, 2:common.VolumeAccessMode mode),

    oneway void volumeContents(1:RequestId requestId, 2:list<common.BlobDescriptor> blobs, 3:list<string> skippedPrefixes),

    oneway void setVolumeMetadataResponse(1:RequestId requestId),

    oneway void getVolumeMetadataResponse(1:RequestId requestId, 2:map<string, string> metadata),

    oneway void statBlobResponse(1:RequestId requestId, 2:common.BlobDescriptor response),

    oneway void startBlobTxResponse(1:RequestId requestId, 2:TxDescriptor response),

    oneway void commitBlobTxResponse(1:RequestId requestId),

    oneway void abortBlobTxResponse(1:RequestId requestId),

    oneway void getBlobResponse(1:RequestId requestId, 2:binary response),

    oneway void getBlobWithMetaResponse(1:RequestId requestId, 2:binary data, 3:common.BlobDescriptor blobDesc),

    oneway void updateMetadataResponse(1:RequestId requestId),

    oneway void renameBlobResponse(1:RequestId requestId, 2:common.BlobDescriptor response),

    oneway void updateBlobResponse(1:RequestId requestId),

    oneway void updateBlobOnceResponse(1:RequestId requestId),

    oneway void deleteBlobResponse(1:RequestId requestId),

    oneway void volumeStatus(1:RequestId requestId, 2:VolumeStatus response),

    oneway void completeExceptionally(1:RequestId requestId, 2:common.ErrorCode errorCode, 3:string message)
}
