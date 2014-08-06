namespace cpp fds.apis
namespace java com.formationds.apis

enum VolumeType {
     OBJECT = 0,
     BLOCK = 1
}

struct VolumeSettings {
       1: required i32 maxObjectSizeInBytes,
       2: required VolumeType volumeType,
       3: required i64 blockDeviceSizeInBytes
}

struct VolumeDescriptor {
       1: required string name,
       2: required i64 dateCreated,
       3: required VolumeSettings policy
}

struct VolumeStatus {
       1: required i64 blobCount
       2: required i64 currentUsageInBytes;
}

struct ObjectOffset {
       1: required i64 value
}

enum ErrorCode {
     INTERNAL_SERVER_ERROR = 0,
     MISSING_RESOURCE,
     BAD_REQUEST,
     RESOURCE_ALREADY_EXISTS,
     RESOURCE_NOT_EMPTY,
     SERVICE_NOT_READY     
}

exception ApiException {
       1: string message;
       2: ErrorCode errorCode;
}

struct BlobDescriptor {
       1: required string name,
       2: required i64 byteCount,
       4: required map<string, string> metadata
}

struct TxDescriptor {
       1: required i64 txId
}

service AmService {
	void attachVolume(1: string domainName, 2:string volumeName)
             throws (1: ApiException e),

        list<BlobDescriptor> volumeContents(1:string domainName, 2:string volumeName, 3:i32 count, 4:i64 offset)
             throws (1: ApiException e),

        BlobDescriptor statBlob(1: string domainName, 2:string volumeName, 3:string blobName)
             throws (1: ApiException e),

        TxDescriptor startBlobTx(1:string domainName, 2:string volumeName, 3:string blobName, 4:i32 blobMode)
	    throws (1: ApiException e),

        void commitBlobTx(1:string domainName, 2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc)
	    throws (1: ApiException e),

        void abortBlobTx(1:string domainName, 2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc)
	    throws (1: ApiException e),

        binary getBlob(1:string domainName, 2:string volumeName, 3:string blobName, 4:i32 length, 5:ObjectOffset offset)
             throws (1: ApiException e),

        void updateMetadata(1:string domainName, 2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc, 5:map<string, string> metadata)
             throws (1: ApiException e),

        void updateBlob(1:string domainName, 
          2:string volumeName, 3:string blobName, 4:TxDescriptor txDesc, 5:binary bytes, 6:i32 length, 7:ObjectOffset objectOffset, 9:bool isLast) throws (1: ApiException e),

        void updateBlobOnce(1:string domainName, 
          2:string volumeName, 3:string blobName, 4:i32 blobMode, 5:binary bytes, 6:i32 length, 7:ObjectOffset objectOffset, 8:map<string, string> metadata) throws (1: ApiException e),
          
          void deleteBlob(1:string domainName, 2:string volumeName, 3:string blobName)
            throws (1: ApiException e)

        VolumeStatus volumeStatus(1:string domainName, 2:string volumeName)
             throws (1: ApiException e),
}


service ConfigurationService {
        void createVolume(1:string domainName, 2:string volumeName, 3:VolumeSettings volumeSettings)
             throws (1: ApiException e),

        void deleteVolume(1:string domainName, 2:string volumeName)
             throws (1: ApiException e),

        VolumeDescriptor statVolume(1:string domainName, 2:string volumeName)
             throws (1: ApiException e),

        list<VolumeDescriptor> listVolumes(1:string domainName)
             throws (1: ApiException e),
}
