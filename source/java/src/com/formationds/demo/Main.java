package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.Configuration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.LocalAmShim;
import org.joda.time.Duration;

public class Main {
    public static final String DEMO_DOMAIN = "demo";

    public static void main(String[] args) throws Exception {
        Configuration config = new Configuration(args);
        new Main().start(8888);
    }

    public void start(int port)  {
        LocalAmShim shim = new LocalAmShim(DEMO_DOMAIN);
        shim.createDomain(DEMO_DOMAIN);

        Xdi xdi = new Xdi(shim);
        WebApp webApp = new WebApp();
        TransientState state = new TransientState(Duration.standardMinutes(5), xdi);

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
    /*
       Returns a jason frame like so:

       [
          {
             operation: "Read performance",
             unit: "IOPs/second"
             values:
             {
                "Volume1": 43,
                "Volume2": 5,
                "Volume3": 12,
             }
          },
          {
             operation: "Write performance",
             unit: "IOPs/second"
             values:
             {
                "Volume1": 43,
                "Volume2": 5,
                "Volume3": 12,
             }
          }
       ]
*/
        webApp.start(port);
    }
}
