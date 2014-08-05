#!/usr/bin/python
import requests

BASE_URL = 'http://localhost:8000/'

def create_bucket(bucket_name):
    return requests.put(BASE_URL+bucket_name)

def delete_bucket(bucket_name):
    return requests.delete(BASE_URL+bucket_name)

def get_bucket(bucket_name):
    return requests.get(BASE_URL+bucket_name)

def put_object(data, bucket, dest):
    return requests.put(BASE_URL+bucket+'/'+dest, data=data)

def get_object(bucket, name):
    return requests.get(BASE_URL+bucket+'/'+name)

def delete_object(bucket, name):
    return requests.delete(BASE_URL+bucket+'/'+name)

def main():
    # Test bucket functionality
    print 'Testing bucket functionality...'
    for i in range(5):
        assert(create_bucket('bucket1').status_code == 200)
        assert(get_bucket('bucket1').status_code == 200)
        assert(delete_bucket('bucket1').status_code == 200)
        assert(get_bucket('bucket1').status_code != 200)
        assert(delete_bucket('bucket1').status_code != 200)
        assert(create_bucket('bucket1').status_code == 200)
        assert(delete_bucket('bucket1').status_code == 200)

    print 'Testing object functionality...'
    assert(create_bucket('bucket2').status_code == 200)
    for i in range(5):
        assert(put_object('abc','bucket2','obj1').status_code == 200)
        assert(get_object('bucket2','obj1').status_code == 200)
        assert(delete_object('bucket2','obj1').status_code == 200)

        assert(get_object('bucket2','obj1').status_code != 200)
        assert(delete_object('bucket2','obj1').status_code != 200)

    print 'Deleting a bucket while full'
    assert(create_bucket('bucket3').status_code == 200)
    assert(put_object('abcd','bucket3','obj1').status_code == 200)
    assert(delete_bucket('bucket3').status_code != 200)

if __name__ == '__main__':
    main()
