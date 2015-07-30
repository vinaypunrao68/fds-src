package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.formationds.om.webkit.rest.LandingPage;
import com.formationds.util.Configuration;
import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.joda.time.Duration;

public class Main {
    public static final String DEMO_DOMAIN = "demo";
    public static final String[] VOLUMES =
        new String[] {
            "fdspanda",
            "fdslemur",
            "fdsfrog",
            "fdsmuskrat"
        };

    public static void main(String[] args) throws Exception {

        ParsedConfig demoConfig = new Configuration("demo", args).getDemoConfig();
        new Main().start(demoConfig);

    }

    public void start(ParsedConfig d) throws Exception {

        DemoConfig demoConfig = new DemoConfig(
                d.defaultString("fds.demo.am_host", "localhost" ),
                d.defaultInt( "fds.demo.swift_port", 9999 ),
                d.defaultInt( "fds.demo.s3_port", 8000 ),
                new BasicAWSCredentials(
                        d.defaultString( "fds.demo.aws_id",
                                         "AKIAINOGA4D75YX26VXQ" ),
                        d.defaultString( "fds.demo.aws_secret",
                                         "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" ) ),
                VOLUMES
        );

        DemoState state = new RealDemoState( Duration.standardSeconds(30),
                                             demoConfig);

        String webDir = d.defaultString( "fds.demo.web_dir",
                                         "../Build/linux-x86_64.debug/lib/demo/" );
        int webappPort = d.defaultInt( "fds.demo.webapp_port", 8888 );

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

        webApp.start(new HttpConfiguration(webappPort, "0.0.0.0"));
    }
}
