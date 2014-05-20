package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.formationds.om.LandingPage;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.joda.time.Duration;

import java.util.Arrays;

public class Main {
    public static final String DEMO_DOMAIN = "demo";
    public static final String[] VOLUMES = new String[]{"fdspanda", "fdslemur", "fdsfrog", "fdsmuskrat"};

    public static void main(String[] args) throws Exception {
        new Main().start(args);
    }

    public void start(String[] args) throws Exception {
        Configuration configuration = new Configuration("demo", args);
        ParsedConfig d = configuration.getDemoConfig();

        DemoConfig demoConfig = new DemoConfig(
                d.lookup("fds.demo.am_host").stringValue(),
                d.lookup("fds.demo.swift_port").intValue(),
                d.lookup("fds.demo.s3_port").intValue(),
                new BasicAWSCredentials(
                        d.lookup("fds.demo.aws_id").stringValue(),
                        d.lookup("fds.demo.aws_secret").stringValue()
                ),
                VOLUMES
        );


        createLocalVolumes(demoConfig);
//        createAwsVolumes(demoConfig);

        DemoState state = new RealDemoState(Duration.standardMinutes(1), demoConfig);

        String webDir = d.lookup("fds.demo.web_dir").stringValue();
        int webappPort = d.lookup("fds.demo.webapp_port").intValue();

        WebApp webApp = new WebApp(webDir);
        webApp.route(HttpMethod.GET, "/", () -> new LandingPage(webDir));

        // POST query string (URL string, q=foo)
        // return 200 OK, body unspecified
        webApp.route(HttpMethod.POST, "/demo/search", () -> new SetCurrentSearch(state));
        webApp.route(HttpMethod.GET, "/demo/setCurrentSearch", () -> new SetCurrentSearch(state));

        // return 200 OK, {q: "pandas eating bamboo" } or 404 if app not started
        webApp.route(HttpMethod.GET, "/demo/search", () -> new GetCurrentSearch(state));

        // Returns {url:"http://something/panda.jpg"} or 404 if app not started
        webApp.route(HttpMethod.GET, "/demo/peekWriteQueue", () -> new PeekWriteQueue(state));

        // Returns {url:"http://something/panda.jpg"} or 404 if app not started
        webApp.route(HttpMethod.GET, "/demo/peekReadQueue", () -> new PeekReadQueue(state));

        // Returns 200 OK, body unspecified
        webApp.route(HttpMethod.POST, "/demo/throttle/reads/:value", () -> new SetThrottle(state, Throttle.read));

        // Returns 200 OK, {value: 42, unit: "IO operations/s" }
        webApp.route(HttpMethod.GET, "/demo/throttle/reads", () -> new GetThrottle(state, Throttle.read));

        // Returns 200 OK, body unspecified
        webApp.route(HttpMethod.POST, "/demo/throttle/writes/:value", () -> new SetThrottle(state, Throttle.write));

        // Returns 200 OK, {value: 42, unit: "IO operations/s" }
        webApp.route(HttpMethod.GET, "/demo/throttle/writes", () -> new GetThrottle(state, Throttle.write));

        webApp.route(HttpMethod.GET, "/demo/pollStats", () -> new PerfStats(state));

        // #POST, "/demo/adapter?type=amazonS3"
        // return 200 OK, {"type": "amazonS3"}
        webApp.route(HttpMethod.POST, "/demo/adapter", () -> new SetObjectStore(state));

        // #GET, "/demo/adapter"
        // return 200 OK, {"type": "amazonS3"}
        webApp.route(HttpMethod.GET, "/demo/adapter", () -> new GetObjectStore(state));

        webApp.start(webappPort);
    }

    private void createAwsVolumes(DemoConfig demoConfig) {
        AmazonS3Client client = new AmazonS3Client(demoConfig.getCreds());
        createVolumes(client, VOLUMES);
    }

    private void createLocalVolumes(DemoConfig demoConfig){
        AmazonS3Client client = new AmazonS3Client(demoConfig.getCreds());
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://" + demoConfig.getAmHost() + ":" + demoConfig.getS3Port());
        createVolumes(client, VOLUMES);
    }

    private void createVolumes(AmazonS3Client client, String[] bucketNames) {
        Arrays.stream(bucketNames)
                .forEach(b -> {
                    try {
                        client.createBucket(b);
                        Thread.sleep(1000);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                });
    }
}
