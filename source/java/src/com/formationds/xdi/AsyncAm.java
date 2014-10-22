package com.formationds.xdi;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class AsyncAm {

    /*
        oneway void statBlob(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName),
        oneway void statBlobResponse(1:RequestId requestId, 2:BlobDescriptor response),

        oneway void startBlobTx(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName, 5:i32 blobMode),
        oneway void startBlobTxResponse(1:RequestId requestId, 2:TxDescriptor response),

        oneway void commitBlobTx(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc),
        oneway void commitBlobTxResponse(1:RequestId requestId),

        oneway void abortBlobTx(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc),
        oneway void abortBlobTxResponse(1:RequestId requestId),

        oneway void getBlob(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName, 5:i32 length, 6:ObjectOffset offset),
        oneway void getBlobResponse(1:RequestId requestId, 2:binary response),

        oneway void updateMetadata(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc, 6:map<string, string> metadata),
        oneway void updateMetadataResponse(1:RequestId requestId),

        oneway void updateBlob(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName, 5:TxDescriptor txDesc, 6:binary bytes,
               7:i32 length, 8:ObjectOffset objectOffset, 9:bool isLast),
        oneway void updateBlobResponse(1:RequestId requestId),

        oneway void updateBlobOnce(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName, 5:i32 blobMode, 6:binary bytes,
               7:i32 length, 8:ObjectOffset objectOffset, 9:map<string, string> metadata),
        oneway void updateBlobOnceResponse(1:RequestId requestId),

        oneway void deleteBlob(1:RequestId requestId, 2:string domainName,
               3:string volumeName, 4:string blobName),
        oneway void deleteBlobResponse(1:RequestId requestId),

        oneway void volumeStatus(1:RequestId requestId, 2:string domainName, 3:string volumeName)
        oneway void volumeStatus(1:RequestId requestId, 2:VolumeStatus response),

        oneway void completeExceptionally(1:RequestId requestId, 2:ErrorCode errorCode, 3:string message)



     */
}
