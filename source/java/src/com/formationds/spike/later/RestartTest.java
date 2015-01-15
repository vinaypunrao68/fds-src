package com.formationds.spike.later;

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import org.apache.commons.io.IOUtils;

public class RestartTest {
    public static void main(String[] args) throws Exception {
        String login = "admin";
        String token = "5304729ba186ce778724bb9196699f4f9d14ba77e999b45afd6c91b8020fc1810bf2dbf881b36bc51b76c40a79d7a368cb0247920ffdba2098b794a015ca2fcf";
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials(login, token));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:8000");

        //client.createBucket("fabrice");
        //client.putObject("fabrice", "panda.txt", new ByteArrayInputStream("hello".getBytes()), new ObjectMetadata());
        //client.listObjects("upgradetest123").getObjectSummaries().forEach(o -> System.out.println(o.getKey()));
        //client.listBuckets().forEach(b -> System.out.println(b.getName()));

        S3ObjectInputStream objectContent = client.getObject("upgradetest123", "upgradetestObject123").getObjectContent();
        String contents = IOUtils.toString(objectContent);
        System.out.println("contents = " + contents);
        //upgradetestObject123
    }
}
