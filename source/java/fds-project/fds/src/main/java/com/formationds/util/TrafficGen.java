/*
 * ====================================================================
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 */

package com.formationds.util;

import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.BasicAWSCredentials;
import com.formationds.util.s3.auth.S3SignatureGeneratorV2;
import org.apache.commons.cli.*;
import org.apache.http.ConnectionReuseStrategy;
import org.apache.http.HttpHost;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.concurrent.FutureCallback;
import org.apache.http.config.ConnectionConfig;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.impl.DefaultConnectionReuseStrategy;
import org.apache.http.impl.NoConnectionReuseStrategy;
import org.apache.http.impl.nio.DefaultHttpClientIODispatch;
import org.apache.http.impl.nio.pool.BasicNIOConnPool;
import org.apache.http.impl.nio.reactor.DefaultConnectingIOReactor;
import org.apache.http.message.BasicHttpEntityEnclosingRequest;
import org.apache.http.message.BasicHttpRequest;
import org.apache.http.nio.protocol.BasicAsyncRequestProducer;
import org.apache.http.nio.protocol.BasicAsyncResponseConsumer;
import org.apache.http.nio.protocol.HttpAsyncRequestExecutor;
import org.apache.http.nio.protocol.HttpAsyncRequester;
import org.apache.http.nio.reactor.ConnectingIOReactor;
import org.apache.http.nio.reactor.IOEventDispatch;
import org.apache.http.protocol.*;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.*;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;


/**
 * Minimal asynchronous HTTP/1.1 client.
 * <p/>
 * Please note that this example represents a minimal HTTP client implementation.
 * It does not support HTTPS as is.
 * You either need to provide BasicNIOConnPool with a connection factory
 * that supports SSL or use a more complex HttpAsyncClient.
 *
 * @see BasicNIOConnPool(org.apache.http.nio.reactor.ConnectingIOReactor,
 * org.apache.http.nio.pool.NIOConnFactory, int)
 * @see org.apache.http.impl.nio.pool.BasicNIOConnFactory
 */
public class TrafficGen {

    static AtomicInteger issued = new AtomicInteger(0);
    static AtomicInteger successes = new AtomicInteger(0);
    static AtomicInteger failures = new AtomicInteger(0);
    static AtomicInteger http_errors = new AtomicInteger(0);
    static AtomicInteger keepalive = new AtomicInteger(0);
    static AtomicLong total_latency = new AtomicLong(0);

    static String test_type = "GET";
    static String volume_name = "volume0";
    static String hostname = "localhost";
    static int n_reqs = 10000;
    static int n_files = 1000;
    static int slots = 100;
    static int osize = 4096;
    static int n_conns = 2;
    static boolean no_reuse = false;
    static int port = 8000;
    static long runtime = 0;
    static long timeout = Long.MAX_VALUE;

    private static String username;
    private static String token;

    static boolean test_timeout = false;

    static Random _gen = new Random(123);
    static byte [] random_buf = null;
    static Integer random_buf_index = 0;

    static class TimeoutTimerTask extends TimerTask {
        @Override
        public void run() {
            test_timeout = true;
        }
    }

    static public BasicHttpEntityEnclosingRequest createPutRequest(int size, String vol, String obj, int n_objs) throws FileNotFoundException, IOException {
        if (random_buf == null) {
            try {
                random_buf = new byte[size * n_objs];
            } catch (NegativeArraySizeException e) {
                System.err.println("size * n_objs must be less than 2^31");
                System.exit(1);
            }
            _gen.nextBytes(random_buf);
        }
        int index;
        synchronized (random_buf_index) {
            if (size * n_objs - random_buf_index < size) {
                random_buf_index = 0;
            }
            index = random_buf_index;
            random_buf_index += size;
        }
        byte[] buf = new byte[size];
        System.arraycopy(random_buf, index, buf, 0, size);
        ByteArrayEntity entity = new ByteArrayEntity(buf);
        BasicHttpEntityEnclosingRequest req = new BasicHttpEntityEnclosingRequest("PUT", "/" + vol + "/" + obj);
        req.setEntity(entity);
        return req;
    }

    public static void main(String[] args) throws Exception {
        Options options = new Options();
        options.addOption(OptionBuilder.withArgName("n_reqs")
                .hasArg()
                .withDescription("Number of requests")
                .create("n_reqs"));
        options.addOption(OptionBuilder.withArgName("object_size")
                .hasArg()
                .withDescription("Number of requests")
                .create("object_size"));
        options.addOption(OptionBuilder.withArgName("outstanding_reqs")
                .hasArg()
                .withDescription("Maximum number of outstanding requests")
                .create("outstanding_reqs"));
        options.addOption(OptionBuilder.withArgName("n_files")
                .hasArg()
                .withDescription("Number of files")
                .create("n_files"));
        options.addOption(OptionBuilder.withArgName("test_type")
                .hasArg()
                .withDescription("Test type")
                .create("test_type"));
        options.addOption(OptionBuilder.withArgName("volume_name")
                .hasArg()
                .withDescription("Volume name")
                .create("volume_name"));
        options.addOption(OptionBuilder.withArgName("hostname")
                .hasArg()
                .withDescription("Host name")
                .create("hostname"));
        options.addOption(OptionBuilder.withArgName("port")
                .hasArg()
                .withDescription("Port")
                .create("port"));
        options.addOption(OptionBuilder.withArgName("n_conns")
                .hasArg()
                .withDescription("Number of NIO connections")
                .create("n_conns"));
        options.addOption("no_reuse", false, "Disable connection reuse");
        options.addOption(OptionBuilder.withArgName("username")
                .isRequired(false)
                .hasArg()
                .withDescription("User name")
                .create("username"));
        options.addOption(OptionBuilder.withArgName("token")
                .isRequired(false)
                .hasArg()
                .withDescription("S3 Authentication token")
                .create("token"));
        options.addOption(OptionBuilder.withArgName("runtime")
                .hasArg()
                .withDescription("Runtime (in seconds)")
                .create("runtime"));
        options.addOption(OptionBuilder.withArgName("timeout")
                .hasArg()
                .withDescription("Timeout")
                .create("timeout"));
        HelpFormatter formatter = new HelpFormatter();

        if (args.length == 0) {
            formatter.printHelp( "trafficgen", options );
            System.exit(0);
        }

        CommandLineParser parser = new BasicParser();


        try {
            CommandLine line = parser.parse(options, args);
            if (line.hasOption("n_reqs")) {
                n_reqs = Integer.parseInt(line.getOptionValue("n_reqs"));
            }
            if (line.hasOption("n_files")) {
                n_files = Integer.parseInt(line.getOptionValue("n_files"));
            }
            if (line.hasOption("outstanding_reqs")) {
                slots = Integer.parseInt(line.getOptionValue("outstanding_reqs"));
            }
            if (line.hasOption("object_size")) {
                osize = Integer.parseInt(line.getOptionValue("object_size"));
            }
            if (line.hasOption("test_type")) {
                test_type = line.getOptionValue("test_type");
            }
            if (line.hasOption("volume_name")) {
                volume_name = line.getOptionValue("volume_name");
            }
            if (line.hasOption("hostname")) {
                hostname = line.getOptionValue("hostname");
            }
            if (line.hasOption("port")) {
                port = Integer.parseInt(line.getOptionValue("port"));
            }
            if (line.hasOption("n_conns")) {
                n_conns = Integer.parseInt(line.getOptionValue("n_conns"));
            }
            if (line.hasOption("no_reuse")) {
                no_reuse = true;
            }
            if (line.hasOption("username")) {
                username = line.getOptionValue("username");
            }
            if (line.hasOption("token")) {
                token = line.getOptionValue("token");
            }
            if (line.hasOption("runtime")) {
                runtime = Integer.parseInt(line.getOptionValue("runtime"));
            }
            if (line.hasOption("timeout")) {
                timeout = Integer.parseInt(line.getOptionValue("timeout"));
            }
        } catch (ParseException exp) {
            // oops, something went wrong
            formatter.printHelp("trafficgen", options );
            System.exit(0);
        }

        System.out.println("n_reqs: " + n_reqs);
        System.out.println("object_size: " + osize);
        System.out.println("outstanding: " + slots);
        System.out.println("n_files: " + n_files);
        System.out.println("volume_name: " + volume_name);
        System.out.println("test_type: " + test_type);
        System.out.println("n_conns: " + n_conns);
        System.out.println("hostname: " + hostname);
        System.out.println("port: " + port);
        System.out.println("no_reuse: " + no_reuse);
        System.out.println("runtime: " + runtime);
        System.out.println("timeout: " + timeout);

        // Create HTTP protocol processing chain
        HttpProcessor httpproc = HttpProcessorBuilder.create()
                // Use standard client-side protocol interceptors
                .add(new RequestContent())
                .add(new RequestTargetHost())
                .add(new RequestConnControl())
                .add(new RequestUserAgent("Test/1.1"))
                .add(new RequestExpectContinue(true))
                .build();
        // HttpProcessor httpproc = HttpProcessorBuilder.create().build();
        ConnectionConfig connectionConfig = ConnectionConfig.DEFAULT;
        System.out.println("connectionConfig: " + connectionConfig.toString());
        // Create client-side HTTP protocol handler
        HttpAsyncRequestExecutor protocolHandler = new HttpAsyncRequestExecutor();
        // Create client-side I/O event dispatch
        final IOEventDispatch ioEventDispatch = new DefaultHttpClientIODispatch(protocolHandler, connectionConfig);
        // Create client-side I/O reactor
        final ConnectingIOReactor ioReactor = new DefaultConnectingIOReactor();
        // Create HTTP connection pool
        BasicNIOConnPool pool = new BasicNIOConnPool(ioReactor, connectionConfig);
        // FIXME: This comment is incorrect by default. See declaration of n_conns.
        // Limit total number of connections to just ten (it was two)
        pool.setDefaultMaxPerRoute(n_conns);
        pool.setMaxTotal(n_conns);
        // Run the I/O reactor in a separate thread
        Thread t = new Thread(new Runnable() {

            public void run() {
                try {
                    // Ready to go!
                    ioReactor.execute(ioEventDispatch);
                } catch (InterruptedIOException ex) {
                    System.err.println("Interrupted");
                } catch (IOException e) {
                    System.err.println("I/O error: " + e.getMessage());
                }
                System.out.println("Shutdown");
            }

        });
        // Start the client thread
        t.start();
        // Create HTTP requester
        ConnectionReuseStrategy connectionReuseStrategy = null;
        if (no_reuse == true) {
            connectionReuseStrategy = new NoConnectionReuseStrategy();
        } else {
            connectionReuseStrategy = new DefaultConnectionReuseStrategy();
        }
        assert connectionReuseStrategy != null;
        HttpAsyncRequester requester = new HttpAsyncRequester(httpproc, connectionReuseStrategy);
        // Execute HTTP GETs to the following hosts and
        HttpHost target = new HttpHost(hostname, port, "http");

        Timer timer = new Timer();
        if (runtime > 0) {
            timer.schedule(new TimeoutTimerTask(), runtime * 1000);
        }

        final CountDownLatch latch = new CountDownLatch(n_reqs);
        long startTime = System.nanoTime();
        System.out.println("Start test - timestamp: " + startTime);
        Semaphore semaphore = new Semaphore(slots);

        for (int j = 0; j < n_reqs; j++) {
            HttpRequest request;
            int obj_index = j % n_files;
            semaphore.acquire();
            switch (test_type) {
                case "GET":
                    request = new BasicHttpRequest(test_type, "/" + volume_name + "/file" + obj_index);
                    if (username != "" && token != "") {
                        request = sign(request);
                    }
                    break;
                case "PUT":
                    request = createPutRequest(osize, volume_name, "file" + obj_index, n_files);
                    if (username != "" && token != "") {
                        request = sign(request);
                    }
                    break;
                default:
                    throw new Exception("Type of this test is not supported: " + test_type);
            }

            if (runtime > 0 && test_timeout) {
                for (int i = 0; i < n_reqs - issued.get(); i++) {
                    latch.countDown();
                }
                break;
            }
            HttpCoreContext coreContext = HttpCoreContext.create();
            // System.out.println("Request: " + request.toString());
            // Thread.sleep(100);
            final long[] reqStartTime = new long[1];
            issued.incrementAndGet();
            requester.execute(
                    new BasicAsyncRequestProducer(target, request) {
                        @Override
                        public synchronized HttpRequest generateRequest() {
                            reqStartTime[0] = System.nanoTime();
                            return super.generateRequest();
                        }
                    },
                    new BasicAsyncResponseConsumer(),
                    pool,
                    coreContext,
                    // Handle HTTP response from a callback
                    new FutureCallback<HttpResponse>() {
                        public void completed(final HttpResponse response) {
                            long reqEndTime = System.nanoTime();
                            total_latency.addAndGet(reqEndTime - reqStartTime[0]);
                            semaphore.release();
                            keepalive.incrementAndGet();
                            latch.countDown();
                            int response_code = response.getStatusLine().getStatusCode();
                            if (response_code != 200) {
                                http_errors.incrementAndGet();
                                failures.incrementAndGet();
                            } else {
                                successes.incrementAndGet();
                            }
                            // System.out.println(target + "->" + response.getStatusLine());
                        }

                        public void failed(final Exception ex) {
                            semaphore.release();
                            failures.incrementAndGet();
                            latch.countDown();
                        }

                        public void cancelled() {
                            semaphore.release();
                            failures.incrementAndGet();
                            latch.countDown();
                        }
                    });
        }
        latch.await(timeout, TimeUnit.SECONDS);

        long endTime = System.nanoTime();
        System.out.println("End test - timestamp: " + endTime);

        long total_time = endTime - startTime;

        System.out.println("Total time [ms]: " + total_time / 1e6);
        System.out.println("IOPs: " + (double) n_reqs / total_time * 1e9);
        System.out.println("successes: " + successes);
        System.out.println("failures: " + failures);
        System.out.println("Http errors: " + http_errors);

        System.out.println("keepalive: " + keepalive);
        System.out.println("Average latency [ns]: " + total_latency.doubleValue() / successes.doubleValue());

        //rf.close();
        timer.cancel();
        System.out.println("Shutting down I/O reactor");
        ioReactor.shutdown();
        System.out.println("Done");
        if (failures.get() > 0) {
            System.exit(1);

        }
    }

    private static HttpRequest sign(HttpRequest request) {
        if (username != null) {
            AWSCredentials creds = new BasicAWSCredentials(username, token);
            String signature = S3SignatureGeneratorV2.hash(request, creds);
            request.addHeader("Authorization", signature);
        }

        return request;
    }

}

