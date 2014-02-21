#!/usr/bin/env python
import sys
import os
import unittest
import time
try:
    import requests
except ImportError:
    print 'oops!! - I need the [requests] pkg. do: "sudo easy_install requests" OR "pip install requests"'
    sys.exit(0)

class Api:
    def __init__(self):
        self.host = 'localhost'
        self.port = 8000
        self.ua = 'fdstest/1.0'
        self.postheaders={'content-type':'application/x-www-form-urlencoded','User-Agent':self.ua}
        self.headers={'User-Agent':self.ua}
        if 'testhost' in os.environ:
            host=os.environ['testhost']
            port=8000
            if ':' in host:
                host,port = host.split(':')
            self.f.host=host
            self.f.port=port

    def getUrl(self,bucket=None,objName=None):
        if bucket == None:
            return 'http://%s:%s' %(self.host,str(self.port))

        if objName == None:
            return 'http://%s:%s/%s' %(self.host,str(self.port), bucket)

        return 'http://%s:%s/%s/%s' %(self.host,str(self.port), bucket, objName)

    def getBucketList(self):
        return requests.post(self.getUrl(),headers=self.headers).text

    def createBucket(self,bucket):
        return requests.post(self.getUrl(bucket),headers=self.headers)

    def getBucket(self,bucket):
        return requests.get(self.getUrl(bucket),headers=self.headers).text

    def deleteBucket(self,bucket):
        return requests.delete(self.getUrl(bucket),headers=self.headers)

    def putFile(self,bucket,filepath,name=None):
        name = name if name else os.path.basename(filepath)
        return requests.post(self.getUrl(bucket,name),data=open(filepath,'rb'),headers=self.postheaders)

    def putObject(self,bucket,objname,data):
        return requests.post(self.getUrl(bucket,objname),data=data,headers=self.postheaders)

    def getObject(self,bucket,objname):
        return requests.get(self.getUrl(bucket,objname),headers=self.headers).text

    def deleteObject(self,bucket,objname):
        return requests.delete(self.getUrl(bucket,objname),headers=self.headers)


class basic(unittest.TestCase):
    def setUp(self):
        self.f=Api()
        
    def testCreateBucket(self):
        r=self.f.createBucket('test_bucket')
        self.assertIn(r.status_code,[200,500])

    def testPutObject(self):
        obj = 'This is a test object'
        bucket='test_bucket'
        objname='test_obj'
        r=self.f.createBucket(bucket)
        self.assertIn(r.status_code,[200,500])

        r=self.f.putObject(bucket,objname,obj)
        self.assertEqual(r.status_code,200)

        time.sleep(2)
        text=self.f.getObject(bucket,objname)
        self.assertEqual(obj,text)

class delete(unittest.TestCase):
    def setUp(self):
        self.f=Api()

    def testDeleteBucket(self):
        r=self.f.createBucket('test_bucket')
        self.assertIn(r.status_code,[200,500])

        r=self.f.deleteBucket('test_bucket')
        self.assertIn(r.status_code,[200])

    def testDeleteObject(self):
        obj = 'This is a test object'
        bucket='test_bucket'
        objname='test_obj'
        r=self.f.createBucket(bucket)
        self.assertIn(r.status_code,[200,500])

        r=self.f.putObject(bucket,objname,obj)
        self.assertEqual(r.status_code,200)
        
        time.sleep(2)
        text=self.f.getObject(bucket,objname)
        self.assertEqual(obj,text)

        r=self.f.deleteObject(bucket,objname)
        self.assertIn(r.status_code,[200])

class medium(unittest.TestCase):
    def setUp(self):
        self.f=Api()

    # create 100 buckets
    def testCreateMultipleBuckets(self):
        for n in range(0,100):
            r=self.f.createBucket('test_bucket_'+str(n))
            self.assertIn(r.status_code,[200,500])

    # put 10 objects from 1KB to 10KB
    def testPutMediumObjects(self):
        bucket='test_bucket_0'
        objname='test_obj'

        r=self.f.createBucket(bucket)
        self.assertIn(r.status_code,[200,500])

        alphastr = ''.join([chr(n+65) for n in range(0,26)])

        # 1 KB Object
        onekb=''
        for n in range(0,39):
            onekb += alphastr 
            onekb += ','

        obj=''
        # test 1-10 kb
        for n in range(0,10):
            obj += onekb
            r=self.f.putObject(bucket,objname,obj)
            self.assertEqual(r.status_code,200)

            time.sleep(2)
            text=self.f.getObject(bucket,objname)
            self.assertEqual(obj,text)
    
if __name__ == "__main__":
    unittest.main(defaultTest='basic')
    
