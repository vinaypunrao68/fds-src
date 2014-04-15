namespace cpp fds.xdi
namespace java com.formationds.xdi

struct Uuid {
       1:i64 low,
       2:i64 high
}

struct VolumePolicy {
       1: i32 objectSizeInBytes
}

struct VolumeDescriptor {
       1: string name,
       2: Uuid uuid,
       3: i64 dateCreated,
       4: VolumePolicy policy
}

struct VolumeStatus {
       1: i64 blobCount
}

exception FdsException {
       1: string message;
}

struct BlobDescriptor {
       1: required i64 byteCount,
       2: required map<string, string> metadata
}

service AmShim {
        void createVolume(1:string domainName, 2:string volumeName, 3:VolumePolicy volumePolicy)
             throws (1: FdsException e),

        void deleteVolume(1:string domainName, 2:string volumeName)
             throws (1: FdsException e),

        VolumeDescriptor statVolume(1:string domainName, 2:string volumeName)
             throws (1: FdsException e),

        list<VolumeDescriptor> listVolumes(1:string domainName)
             throws (1: FdsException e),

        VolumeStatus volumeStatus(1:string domainName, 2:string volumeName)
             throws (1: FdsException e),

        list<string> volumeContents(1:string domainName, 2:string volumeName, 3:i32 count, 4:i64 offset)
             throws (1: FdsException e),

        BlobDescriptor statBlob(1: string domainName, 2:string volumeName, 3:string blobName)
             throws (1: FdsException e),

        binary getBlob(1:string domainName, 2:string volumeName, 3:string blobName, 4:i32 length, 5:i64 offset)
             throws (1: FdsException e),

        Uuid startBlobTx(1:string domainName, 2:string volumeName, 3:string blobName)
             throws (1: FdsException e),

        void updateMetadata(1:string domainName, 2:string volumeName, 3:string blobName, 5:Uuid txUuid, 6:map<string, string> metadata)
             throws (1: FdsException e),

        void updateBlob(1:string domainName, 2:string volumeName, 3:string blobName, 4:Uuid txUuid, 5:binary bytes, 6:i32 length, 7:i64 offset)
             throws (1: FdsException e),

        void commit(1:Uuid txId)
             throws (1: FdsException e),

        void deleteBlob(1:string domainName, 2:string volumeName, 3:string blobName)
             throws (1: FdsException e)

}
