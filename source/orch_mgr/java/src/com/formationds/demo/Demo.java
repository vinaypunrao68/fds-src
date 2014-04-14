package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.joda.time.Duration;

public class Demo {
    public void start(int port) {
        WebApp webApp = new WebApp();
        TransientState state = new TransientState(Duration.standardMinutes(5));
        // POST query string (URL string, q=foo)
        // return 200 OK, body unspecified
        webApp.route(HttpMethod.POST, "/demo/search", () -> new SetCurrentSearch(state));
        // webApp.route(HttpMethod.GET, "/demo/setCurrentSearch", () -> new SetCurrentSearch(state));

        // return 200 OK, {q: "pandas eating bamboo" } or 404 if app not started
        webApp.route(HttpMethod.GET, "/demo/search", () -> new GetCurrentSearch(state));

        // Returns {url:"http://something/panda.jpg"} or 404 if app not started
        webApp.route(HttpMethod.GET, "/demo/peekWriteQueue", () -> new RandomImage(state));

        // Returns {url:"http://something/panda.jpg"} or 404 if app not started
        webApp.route(HttpMethod.GET, "/demo/peekReadQueue", () -> new RandomImage(state));

        // Returns 200 OK, body unspecified
        webApp.route(HttpMethod.POST, "/demo/throttle/reads/:value", () -> new SetThrottle(state, Throttle.read));

        // Returns 200 OK, {value: 42, unit: "IO operations/s" }
        webApp.route(HttpMethod.GET, "/demo/throttle/reads", () -> new GetThrottle(state, Throttle.read));

        // Returns 200 OK, body unspecified
        webApp.route(HttpMethod.POST, "/demo/throttle/writes/:value", () -> new SetThrottle(state, Throttle.write));

        // Returns 200 OK, {value: 42, unit: "IO operations/s" }
        webApp.route(HttpMethod.GET, "/demo/throttle/writes", () -> new GetThrottle(state, Throttle.write));


        webApp.route(HttpMethod.GET, "/demo/stats", () -> new PerfStats(state));
    /*
       Returns a jason frame like so:

       [
          {
             operation: "Read performance",
             series:
             [
                {
                   name: "Volume 1",
                   values: [2, 5, 745, 8]
                },
                {
                   name: "Volume 2",
                   values: [1, 2, 4, 5]
                }
             ]
          },
          {
             operation: "Write performance",
             series:
             [
                {
                   name: "Volume 1",
                   values: [2, 5, 745, 8]
                },
                {
                   name: "Volume 2",
                   values: [1, 2, 4, 5]
                }
             ]
          }
       ]
*/
        webApp.start(port);
    }
}
