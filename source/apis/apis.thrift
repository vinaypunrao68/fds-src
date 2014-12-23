include "../fdsp/common.thrift"
include "../fdsp/snapshot.thrift"
include "../fdsp/fds_stream.thrift"
namespace cpp fds.apis
namespace java com.formationds.apis

enum VolumeType {
     OBJECT = 0,
     BLOCK = 1
}

struct VolumeSettings {
       1: required i32 maxObjectSizeInBytes,
       2: required VolumeType volumeType,
       3: required i64 blockDeviceSizeInBytes,
       4: required i64 contCommitlogRetention
}

struct VolumeDescriptor {
       1: required string name,
       2: required i64 dateCreated,
       3: required VolumeSettings policy,
       4: required i64 tenantId,
       5: i64 volId,
       6: common.ResourceState state
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

struct RequestId {
       1: required string id;
}

service AsyncAmServiceRequest {
	oneway void handshakeStart(1:RequestId requestId 2:i32 portNumber),

	oneway void attachVolume(1:RequestId requestId, 2: string domainName, 
	       3:string volumeName),

        oneway void volumeContents(1:RequestId requestId, 2:string domainName, 
	       3:string volumeName, 4:i32 count, 5:i64 offset),

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

service AsyncAmServiceResponse {
	oneway void handshakeComplete(1:RequestId requestId),

	oneway void attachVolumeResponse(1:RequestId requestId),

        oneway void volumeContents(1:RequestId requestId, 2:list<BlobDescriptor> response),

        oneway void statBlobResponse(1:RequestId requestId, 2:BlobDescriptor response),

        oneway void startBlobTxResponse(1:RequestId requestId, 2:TxDescriptor response),

        oneway void commitBlobTxResponse(1:RequestId requestId),

	oneway void abortBlobTxResponse(1:RequestId requestId),

        oneway void getBlobResponse(1:RequestId requestId, 2:binary response),

	oneway void getBlobWithMetaResponse(1:RequestId requestId, 2:binary data, 3:BlobDescriptor blobDesc),

        oneway void updateMetadataResponse(1:RequestId requestId),

        oneway void updateBlobResponse(1:RequestId requestId),

        oneway void updateBlobOnceResponse(1:RequestId requestId),

        oneway void deleteBlobResponse(1:RequestId requestId),

        oneway void volumeStatus(1:RequestId requestId, 2:VolumeStatus response),

	oneway void completeExceptionally(1:RequestId requestId, 2:ErrorCode errorCode, 3:string message)
}



// Added for multi-tenancy
struct User {
    1: i64 id,
    2: string identifier,
    3: string passwordHash,
    4: string secret,
    5: bool isFdsAdmin
}

// Added for multi-tenancy
struct Tenant {
    1: i64 id,
    2: string identifier
}


service ConfigurationService {
        // // Added for multi-tenancy
        i64 createTenant(1:string identifier)
             throws (1: ApiException e),

        list<Tenant> listTenants(1:i32 ignore)
             throws (1: ApiException e),

        i64 createUser(1:string identifier, 2: string passwordHash, 3:string secret, 4: bool isFdsAdmin)
             throws (1: ApiException e),

        void assignUserToTenant(1:i64 userId, 2:i64 tenantId)
             throws (1: ApiException e),

        void revokeUserFromTenant(1:i64 userId, 2:i64 tenantId)
             throws (1: ApiException e),

        list<User> allUsers(1:i64 ignore)
             throws (1: ApiException e),

        list<User> listUsersForTenant(1:i64 tenantId)
             throws (1: ApiException e),

        void updateUser(1: i64 userId, 2:string identifier, 3:string passwordHash, 4:string secret, 5:bool isFdsAdmin)
             throws (1: ApiException e),


        // Added for caching
        i64 configurationVersion(1: i64 ignore)
             throws (1: ApiException e),

        void createVolume(1:string domainName, 2:string volumeName, 3:VolumeSettings volumeSettings, 4: i64 tenantId)
             throws (1: ApiException e),

        i64 getVolumeId(1:string volumeName)
             throws (1: ApiException e),

        string getVolumeName(1:i64 volumeId)
             throws (1: ApiException e),

        void deleteVolume(1:string domainName, 2:string volumeName)
             throws (1: ApiException e),

        VolumeDescriptor statVolume(1:string domainName, 2:string volumeName)
             throws (1: ApiException e),

        list<VolumeDescriptor> listVolumes(1:string domainName)
             throws (1: ApiException e),

        i32 registerStream(
             1:string url,
             2:string http_method,
             3:list<string> volume_names,
             4:i32 sample_freq_seconds,
             5:i32 duration_seconds),

        list<fds_stream.StreamingRegistrationMsg> getStreamRegistrations(1:i32 ignore),
        void deregisterStream(1:i32 registration_id),

    i64 createSnapshotPolicy(1:snapshot.SnapshotPolicy policy)
             throws (1: ApiException e),

    list<snapshot.SnapshotPolicy> listSnapshotPolicies(1:i64 unused)
             throws (1: ApiException e),

    void deleteSnapshotPolicy(1:i64 id)
             throws (1: ApiException e),

    void attachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
             throws (1: ApiException e),

    list<snapshot.SnapshotPolicy> listSnapshotPoliciesForVolume(1:i64 volumeId)
             throws (1: ApiException e),

    void detachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
             throws (1: ApiException e),

    list<i64> listVolumesForSnapshotPolicy(1:i64 policyId)
             throws (1: ApiException e),

    void createSnapshot(1:i64 volumeId, 2:string snapshotName, 3:i64 retentionTime, 4:i64 timelineTime)
             throws (1: ApiException e),

    list<snapshot.Snapshot> listSnapshots(1:i64 volumeId)
             throws (1: ApiException e),

    void restoreClone(1:i64 volumeId, 2:i64 snapshotId)
             throws (1: ApiException e),

    // Returns VolumeID of clone 
    i64 cloneVolume(1:i64 volumeId, 2:i64 fdsp_PolicyInfoId, 3:string cloneVolumeName, 4:i64 timelineTime)

}
