#!/usr/bin/python
#
# Tool for testing/demoing multipart upload using the aws s3api shell command
#
import subprocess
import json
import argparse
import os
import tempfile

def execute_with_json_output(cmdlst, verbose):
    if not verbose:
        return json.loads(subprocess.check_output(cmdlst))
    else:
        print "executing: " + " ".join(cmdlst)
        output = subprocess.check_output(cmdlst)
        print "command output:"
        print output
        return json.loads(output)


def build_s3_command(settings, *args):
    return ['aws', '--endpoint-url', settings.endpoint, 's3api'] + list(args)

parser = argparse.ArgumentParser(description='MPU test')
parser.add_argument('-ep', required=True, dest='endpoint', help='AWS endpoint')
parser.add_argument('-v', dest='verbose', action='store_true', help='enable verbose output')
parser.add_argument('-part', dest='partfiles', action='append', required=True, help='files containing parts')
parser.add_argument('-bucket', dest='bucket', required=True, help='aws bucket name')
parser.add_argument('-key', dest='key', required=True, help='aws object key')
settings = parser.parse_args()

print "starting multi-part upload"

create_mpu_cmd = build_s3_command(settings, "create-multipart-upload", "--bucket", settings.bucket, '--key', settings.key)
create_mpu_response = execute_with_json_output(create_mpu_cmd, settings.verbose)

print "multi-part upload started - upload id: " + create_mpu_response['UploadId']

partNumber = 1
upload_id = create_mpu_response['UploadId']

print "uploading parts..."
upload_part_responses = []
for part in settings.partfiles:
    print "uploading part %d (%s)" % (partNumber, part)
    upload_part_cmd = build_s3_command(settings, "upload-part", "--bucket", settings.bucket, "--key", settings.key, "--upload-id", upload_id, "--part-number", str(partNumber), '--body', part)
    upload_part_response = execute_with_json_output(upload_part_cmd, settings.verbose)
    upload_part_sig = {'ETag': upload_part_response['ETag'], 'PartNumber': partNumber}
    upload_part_responses.append(upload_part_sig)
    print "upload part complete"
    partNumber += 1

print "uploading parts complete"

mpu_struct = {"Parts": upload_part_responses}

if settings.verbose:
    print "mpu-struct:"
    print json.dumps(mpu_struct)

with tempfile.NamedTemporaryFile() as mpu_struct_file:
    json.dump(mpu_struct, mpu_struct_file)
    mpu_struct_file.flush()
    complete_cmd = build_s3_command(settings, "complete-multipart-upload", "--bucket", settings.bucket, "--key", settings.key, "--upload-id", upload_id, "--multipart-upload", "file://" + mpu_struct_file.name)
    complete_response = execute_with_json_output(complete_cmd, settings.verbose)

print "multi-part upload completed successfully"
