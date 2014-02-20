package com.formationds.amazons3;

import com.amazonaws.ClientConfiguration;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;

/**
 * Created by fabrice on 2/18/14.
 */
public class SampleS3Client {
    public static void main(String[] args) {
        ClientConfiguration clientConfig = new ClientConfiguration();
        //clientConfig.setProxyHost("localhost");
        //clientConfig.setProxyPort(8888);
        BasicAWSCredentials basicCred = new BasicAWSCredentials("my_access_key", "my_secret_key");
        AmazonS3Client client = new AmazonS3Client(basicCred, clientConfig);
//        AmazonS3Client client = new AmazonS3Client();
        client.setEndpoint("http://localhost:8000/");
        S3ClientOptions options = new S3ClientOptions();
        options.setPathStyleAccess(true);
        client.setS3ClientOptions(options);

        //Bucket myBucket = client.createBucket("mybucket");
        //System.out.println(myBucket.getName());


        client.listBuckets().forEach(b -> System.out.println(b.getName()));
    }
}
