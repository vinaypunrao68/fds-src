#!/usr/bin/python
import os
import math
import boto
from boto.s3.connection import OrdinaryCallingFormat
from filechunkio import FileChunkIO

def main():
    # Connect to FDS S3 interface
    c = boto.connect_s3(
            aws_access_key_id='blablabla',
            aws_secret_access_key='kekekekek',
            host='localhost',
            port=8000,
            is_secure=False,
            calling_format=boto.s3.connection.OrdinaryCallingFormat())

    b = c.create_bucket("bucket1")
    print 'Bucket created!'

    # Get file info
    source_path = '/mup_file'
    source_size = os.stat(source_path).st_size

    # Create multipart upload request
    mp = b.initiate_multipart_upload(os.path.basename(source_path))
    print 'MPU initiated!'

    # Use a chunk size of 50 MB
    chunk_size = 52428800
    chunk_count = int(math.ceil(source_size / chunk_size))

    # Send the file parts, using FileChunkIO to create a file-like object
    # that points to a certain byte range within the original file. We
    # set bytes to never exceed the original file size.
    for i in range(chunk_count + 1):
        offset = chunk_size * i
        bytes = min(chunk_size, source_size - offset)
        with FileChunkIO(source_path, 'r', offset=offset, bytes=bytes) as fp:
            mp.upload_part_from_file(fp, part_num=i + 1)
    print 'MPU Pieces Uploaded!'

    # Finish the upload
    mp.complete_upload()
    print 'MPU Complete!"'

if __name__ == '__main__':
    main()
