package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.util.json.JSONObject;
import com.formationds.smoketest.ConsoleProgress;
import com.formationds.smoketest.HttpClientFactory;
import com.formationds.util.s3.S3SignatureGenerator;
import org.apache.commons.io.IOUtils;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.concurrent.FutureCallback;
import org.apache.http.conn.ssl.AllowAllHostnameVerifier;
import org.apache.http.conn.ssl.SSLContextBuilder;
import org.apache.http.conn.ssl.SSLContexts;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.impl.client.DefaultConnectionKeepAliveStrategy;
import org.apache.http.impl.nio.client.CloseableHttpAsyncClient;
import org.apache.http.impl.nio.client.HttpAsyncClients;
import org.apache.http.impl.nio.reactor.IOReactorConfig;

import javax.net.ssl.SSLContext;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.security.SecureRandom;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;
import java.util.stream.IntStream;

public class AsyncBanger {
    public static final int BLOCKSIZE = 4096;
    private final HttpClient httpClient;
    private final AWSCredentials credentials;
    private final SecureRandom random = new SecureRandom();
    private String s3BaseUrl;

    public AsyncBanger(String omBaseUrl, String s3BaseUrl, String username, String password) throws Exception {
        this.s3BaseUrl = s3BaseUrl;
        httpClient = new HttpClientFactory().makeHttpClient();
        String token = obtainToken(omBaseUrl, username, password);
        credentials = new BasicAWSCredentials(username, token);
    }

    public static void main(String[] args) throws Exception {
        AsyncBanger banger = new AsyncBanger("https://localhost:7443", "https://localhost:8443", "admin", "admin");
        banger.execute(50, 100000);
    }

    public void execute(int concurrency, int totalObjects) throws Exception {
        String bucketName = setup();
        IOReactorConfig config = IOReactorConfig.custom()
                .setIoThreadCount(concurrency)
                .build();

        SSLContextBuilder builder = SSLContexts.custom();
        builder.loadTrustMaterial(null, (chain, authType) -> true);

        SSLContext sslContext = builder.build();

        CloseableHttpAsyncClient asyncClient = HttpAsyncClients.custom()
                .setDefaultIOReactorConfig(config)
                .setHostnameVerifier(new AllowAllHostnameVerifier())
                .setSSLContext(sslContext)
                .setKeepAliveStrategy(new DefaultConnectionKeepAliveStrategy())
                .build();

        asyncClient.start();
        ConsoleProgress progress = new ConsoleProgress("Banging", totalObjects, 100);

        CompletableFuture<HttpResponse>[] futures = IntStream.range(0, totalObjects)
                .mapToObj(i -> scheduleRequests(asyncClient, bucketName, i).thenApply(r -> progress.applyAsInt(i)))
                .toArray(i -> new CompletableFuture[i]);

        CompletableFuture.allOf(futures).get();
        tearDown(bucketName);
        asyncClient.close();
    }

    private CompletableFuture<HttpResponse> scheduleRequests(CloseableHttpAsyncClient asyncClient, String bucketName, int n) {
        String key = Integer.toString(n);
        CompletableFuture<HttpResponse> putFuture = schedulePut(asyncClient, bucketName, key);
        CompletableFuture<HttpResponse> getFuture = putFuture.thenCompose(r -> scheduleGet(asyncClient, bucketName, key));
        CompletableFuture<HttpResponse> deleteFuture = getFuture.thenCompose(r -> scheduleDelete(asyncClient, bucketName, key));
        return deleteFuture;
    }

    private CompletableFuture<HttpResponse> scheduleDelete(CloseableHttpAsyncClient asyncClient, String bucketName, String key) {
        HttpDelete delete = (HttpDelete) sign(new HttpDelete(s3BaseUrl + "/" + bucketName + "/" + key));
        CompletableFuture<HttpResponse> cf = new CompletableFuture<>();
        return executeAsync(asyncClient, delete, cf, response -> {
            if (response.getStatusLine().getStatusCode() != HttpServletResponse.SC_NO_CONTENT) {
                cf.completeExceptionally(new IOException(response.getStatusLine().getStatusCode() + ": " + response.getStatusLine().getReasonPhrase()));
            } else {
                cf.complete(response);
            }
        });
    }

    private CompletableFuture<HttpResponse> scheduleGet(CloseableHttpAsyncClient asyncClient, String bucketName, String key) {
        HttpGet get = (HttpGet) sign(new HttpGet(s3BaseUrl + "/" + bucketName + "/" + key));
        CompletableFuture<HttpResponse> cf = new CompletableFuture<>();
        return executeAsync(asyncClient, get, cf, response -> {
            int statusCode = response.getStatusLine().getStatusCode();
            if (statusCode != HttpServletResponse.SC_OK) {
                cf.completeExceptionally(new IOException(statusCode + ": " + response.getStatusLine().getReasonPhrase()));
            } else {
                try {
                    IOUtils.read(response.getEntity().getContent(), new byte[BLOCKSIZE]);
                    cf.complete(response);
                } catch (IOException e) {
                    cf.completeExceptionally(e);
                }
            }
        });
    }

    private CompletableFuture<HttpResponse> schedulePut(CloseableHttpAsyncClient asyncClient, String bucketName, String key) {
        HttpPut put = (HttpPut) sign(new HttpPut(s3BaseUrl + "/" + bucketName + "/" + key));
        byte[] bytes = new byte[BLOCKSIZE];
        random.nextBytes(bytes);
        put.setEntity(new ByteArrayEntity(bytes));
        CompletableFuture<HttpResponse> cf = new CompletableFuture<>();
        return executeAsync(asyncClient, put, cf, response -> {
            int statusCode = response.getStatusLine().getStatusCode();
            if (statusCode != HttpServletResponse.SC_OK) {
                cf.completeExceptionally(new IOException(statusCode + ": " + response.getStatusLine().getReasonPhrase()));
            } else {
                cf.complete(response);
            }

        });
    }

    private CompletableFuture<HttpResponse> executeAsync(CloseableHttpAsyncClient client, HttpUriRequest request, CompletableFuture<HttpResponse> cf, Consumer<HttpResponse> consumer) {
        client.execute(request, new FutureCallback<HttpResponse>() {
            @Override
            public void completed(HttpResponse httpResponse) {
                consumer.accept(httpResponse);
            }

            @Override
            public void failed(Exception e) {
                cf.completeExceptionally(e);
            }

            @Override
            public void cancelled() {
                cf.completeExceptionally(new InterruptedException());
            }
        });

        return cf;
    }

    private void tearDown(String bucketName) throws IOException {
        HttpDelete delete = (HttpDelete) sign(new HttpDelete(s3BaseUrl + "/" + bucketName));
        HttpResponse response = httpClient.execute(delete);
        throwErrorMaybe(response, HttpServletResponse.SC_NO_CONTENT);
    }

    private String setup() throws IOException {
        String bucketName = UUID.randomUUID().toString();
        HttpPut put = (HttpPut) sign(new HttpPut(s3BaseUrl + "/" + bucketName));
        HttpResponse response = httpClient.execute(put);
        throwErrorMaybe(response, HttpServletResponse.SC_OK);
        return bucketName;
    }

    private void throwErrorMaybe(HttpResponse response, int expected) throws IOException {
        int statusCode = response.getStatusLine().getStatusCode();
        if (statusCode != expected) {
            throw new IOException(statusCode + ": " + response.getStatusLine().getReasonPhrase());
        }
    }

    private HttpRequest sign(HttpRequest request) {
        String hash = S3SignatureGenerator.hash(request, credentials);
        request.addHeader("Authorization", hash);
        return request;
    }

    private String obtainToken(String url, String username, String password) throws Exception {
        HttpGet httpGet = new HttpGet(url + "/api/auth/token?login=" + username + "&password=" + password);
        HttpResponse response = httpClient.execute(httpGet);
        throwErrorMaybe(response, HttpServletResponse.SC_OK);
        JSONObject o = new JSONObject(IOUtils.toString(response.getEntity().getContent()));
        return o.getString("token");
    }
}
