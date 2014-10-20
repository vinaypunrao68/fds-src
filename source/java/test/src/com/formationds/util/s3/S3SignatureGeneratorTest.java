package com.formationds.util.s3;

import com.amazonaws.AmazonClientException;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.*;
import com.formationds.xdi.s3.S3Failure;
import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

public class S3SignatureGeneratorTest {
    @Test
    public void testBasicAuth() throws Exception {
        BasicAWSCredentials credentials = new BasicAWSCredentials("proot preet", "waxy meat");

        try(SignatureGeneratorTestContext ctx = new SignatureGeneratorTestContext(credentials, 10293)) {

            // buckets
            ctx.test(client -> client.createBucket("ptath"));
            ctx.test(client -> client.deleteBucket("zaub"));
            ctx.test(client -> client.listBuckets());
            ctx.test(client -> client.listObjects("buckey"));
            ctx.test(client -> client.listObjects("buckey", "truds"));

            // objects
            ObjectMetadata weirdContentType = new ObjectMetadata();
            weirdContentType.setContentType("SLOPPY PRETZEL IMAGES");


            ctx.test(client -> client.getObject("potato", "ole"));
            ctx.test(client -> client.putObject("muffins", "beans", makeStream(), new ObjectMetadata()));
            ctx.test(client -> client.putObject("muffins", "beans", makeStream(), weirdContentType));
            ctx.test(client -> client.deleteObject("mop", "cand"));
            ctx.test(client -> client.deleteObjects(new DeleteObjectsRequest("mop").withKeys("sped", "maff", "parp")));
            ctx.test(client -> client.copyObject("smoop", "src", "smoop", "dst"));

            // multipart
            ctx.test(client -> client.initiateMultipartUpload(new InitiateMultipartUploadRequest("wax", "breeks")));
            ctx.test(client -> client.uploadPart(new UploadPartRequest().withPartNumber(1).withUploadId("proop").withBucketName("preepys").withKey("meps").withInputStream(makeStream())));
            ctx.test(client -> client.copyPart(new CopyPartRequest()
                    .withPartNumber(1)
                    .withSourceBucketName("proop")
                    .withDestinationBucketName("smuckie")
                    .withSourceKey("preep")
                    .withDestinationKey("chuds")
                    .withUploadId("mops")));
            ctx.test(client -> client.abortMultipartUpload(new AbortMultipartUploadRequest("prap", "flarp", "flarrrn")));
            ctx.test(client -> client.completeMultipartUpload(new CompleteMultipartUploadRequest("prap", "flarp", "flarrrn", new ArrayList<PartETag>())));
            ctx.test(client -> client.listParts(new ListPartsRequest("buckey", "smap", "weefis")));
            ctx.test(client -> client.listMultipartUploads(new ListMultipartUploadsRequest("rooper")));

            // acls
            ctx.test(client -> client.setBucketAcl("wax", CannedAccessControlList.AuthenticatedRead));
            ctx.test(client -> client.setObjectAcl("prap", "skooge", CannedAccessControlList.AuthenticatedRead));
        }
    }

    private InputStream makeStream() {
        return new ByteArrayInputStream(new byte[]{1, 2, 3, 4});
    }

    class SignatureGeneratorTestContext implements AutoCloseable {
        public final AuthorizerReferenceServer authReferenceServer;
        public final AWSCredentials credentials;
        private final AmazonS3Client s3client;
        Exception exc;

        public SignatureGeneratorTestContext(AWSCredentials credentials, int port) throws IOException {
            authReferenceServer = new AuthorizerReferenceServer(port);
            this.credentials = credentials;
            s3client = new AmazonS3Client(credentials);
            s3client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
            s3client.setEndpoint("http://localhost:" + port);
        }

        public void test(Consumer<AmazonS3Client> operation) {
            try {
                operation.accept(s3client);
            } catch(AmazonClientException ex) {
                // do nothing
            }

            assertNotNull(authReferenceServer.getLastAuthToken());
            String testSig = S3SignatureGenerator.hash(
                    authReferenceServer.getLastMethod(),
                    authReferenceServer.getLastPath(),
                    authReferenceServer.getLastHeaders(),
                    authReferenceServer.getLastQueryString(),
                    credentials
            );

            assertEquals(authReferenceServer.getLastAuthToken(), testSig);
        }

        @Override
        public void close() throws Exception {
            authReferenceServer.close();
        }
    }
}

// TODO: should probably use jetty for this test
class AuthorizerReferenceServer implements HttpHandler, AutoCloseable {
    private final HttpServer server;
    private String lastAuthToken;

    private String lastMethod;
    private String lastPath;
    private Map<String, List<String>> lastHeaders;
    private String lastQueryString;

    public AuthorizerReferenceServer(int port) throws IOException {
        server = HttpServer.create(new InetSocketAddress(port), 0);
        server.createContext("/", this);
        server.setExecutor(null);
        server.start();
    }

    @Override
    public void handle(HttpExchange httpExchange) throws IOException {
        Headers hdrs = httpExchange.getRequestHeaders();
        if(hdrs.containsKey("Authorization")) {
            lastAuthToken = hdrs.getFirst("Authorization");
        }

        Map<String, String> headerMap = hdrs.entrySet().stream().collect(Collectors.toMap(e -> e.getKey(), e -> e.getValue().get(0)));
        headerMap.remove("Authorization");
        for(Map.Entry<String, String> e : headerMap.entrySet()) {
            if(e.getValue().isEmpty())
                e.setValue(null);
        }

        S3Failure failure = new S3Failure(S3Failure.ErrorCode.NoSuchKey, "Hello, world!", "foo");
        lastMethod = httpExchange.getRequestMethod();
        lastPath = httpExchange.getRequestURI().getPath();
        lastQueryString = httpExchange.getRequestURI().getQuery();
        lastHeaders = httpExchange.getRequestHeaders();

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        failure.render(baos);
        byte[] bytes = baos.toByteArray();

        httpExchange.sendResponseHeaders(404, bytes.length);
        httpExchange.getResponseBody().write(bytes);
        httpExchange.getResponseBody().close();
    }

    public String getLastAuthToken() {
        return lastAuthToken;
    }

    @Override
    public void close() throws Exception {
        server.stop(0);
    }

    public String getLastMethod() {
        return lastMethod;
    }

    public String getLastPath() {
        return lastPath;
    }

    public Map<String, List<String>> getLastHeaders() {
        return lastHeaders;
    }

    public String getLastQueryString() {
        return lastQueryString;
    }
}