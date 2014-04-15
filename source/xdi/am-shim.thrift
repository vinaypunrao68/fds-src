namespace cpp fds.xdi
namespace java com.formationds.xdi

struct VolumeDescriptor {
       1: required i32 objectSizeInBytes;
}

exception FdsException {
       1: string message;
}

struct BlobDescriptor {
       1: required i64 byteCount,
       2: required map<string, string> metadata
}

service AmShim {
        void createVolume(1:string domainName, 2:string volumeName, 3:VolumeDescriptor volumeDescriptor)
             throws (1: FdsException e),

        void deleteVolume(1:string domainName, 2:string volumeName)
             throws (1: FdsException e),

        VolumeDescriptor statVolume(1:string domainName, 2:string volumeName)
             throws (1: FdsException e),

        list<string> listVolumes(1:string domainName)
             throws (1: FdsException e),

        list<string> volumeContents(1:string domainName, 2:string volumeName)
             throws (1: FdsException e),

        BlobDescriptor statBlob(1: string domainName, 2:string volumeName, 3:string blobName)
             throws (1: FdsException e),

        binary getBlob(1:string domainName, 2:string volumeName, 3:string blobName, 4:i32 length, 5:i64 offset)
             throws (1: FdsException e),

        void updateMetadata(1:string domainName, 2:string volumeName, 3:string blobName, 4:map<string, string> metadata)
             throws (1: FdsException e),

        void updateBlob(1:string domainName, 2:string volumeName, 3:string blobName, 4:binary bytes, 5:i32 length, 6:i64 offset)
             throws (1: FdsException e)
}
