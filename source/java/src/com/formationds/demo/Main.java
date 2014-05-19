package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeConnector;
import com.formationds.apis.VolumePolicy;
import com.formationds.om.LandingPage;
import com.formationds.security.Authenticator;
import com.formationds.security.JaasAuthenticator;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParserFacade;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.ConnectionProxy;
import com.formationds.xdi.Xdi;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransportException;
import org.joda.time.Duration;

public class Main {
    public static final String DEMO_DOMAIN = "demo";

    public static void main(String[] args) throws Exception {
        new Main().start(8888, args);
    }

    public void start(int port, String[] args) throws Exception {
        Configuration configuration = new Configuration(args);
        ParserFacade amConfig = configuration.getPlatformConfig();

        AmService.Iface am = new ConnectionProxy<>(AmService.Iface.class, () -> {
            TSocket amTransport = new TSocket("localhost", 9988);
            try {
                amTransport.open();
            } catch (TTransportException e) {
                throw new RuntimeException(e);
            }
            return new AmService.Client(new TBinaryProtocol(amTransport));
        }).makeProxy();

        ConfigurationService.Iface config = new ConnectionProxy<>(ConfigurationService.Iface.class, () -> {
            String omHost = amConfig.lookup("fds.am.om_ip").stringValue();
            TSocket omTransport = new TSocket(omHost, 9090);
            try {
                omTransport.open();
            } catch (TTransportException e) {
                throw new RuntimeException(e);
            }
            return new ConfigurationService.Client(new TBinaryProtocol(omTransport));
        }).makeProxy();

        Authenticator authenticator = new JaasAuthenticator();
        Xdi xdi = new Xdi(am, config, authenticator);


        String webDir = "/home/fabrice/demo/dist";
        WebApp webApp = new WebApp(webDir);
        String[] volumes = createVolumes(xdi);
//        String[] volumes = xdi.listVolumes(DEMO_DOMAIN)
//                .stream()
//                .map(v -> v.getName())
//                .toArray(i -> new String[i]);

        //DemoState state = new RealDemoState(Duration.standardMinutes(5), new XdiImageReader(xdi), new XdiImageWriter(xdi, volumes));
        DemoState state = new RealDemoState(Duration.standardMinutes(5),
                new S3ImageReader("localhost", 8000),
                new S3ImageWriter("localhost", 8000, volumes));

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

    private String[] createVolumes(Xdi xdi) throws InterruptedException {
        try {
            //xdi.deleteVolume(Main.DEMO_DOMAIN, "Volume1");
            //xdi.deleteVolume(Main.DEMO_DOMAIN, "Volume2");
            //xdi.deleteVolume(Main.DEMO_DOMAIN, "Volume3");
            //xdi.deleteVolume(Main.DEMO_DOMAIN, "Volume4");
        } catch (Exception e) {

        }

        try {
            xdi.createVolume(Main.DEMO_DOMAIN, "Volume1",
                    new VolumePolicy(1024 * 4, VolumeConnector.S3));
            Thread.sleep(1000);
            xdi.createVolume(Main.DEMO_DOMAIN, "Volume2",
                    new VolumePolicy(1024 * 4, VolumeConnector.S3));
            Thread.sleep(1000);
            xdi.createVolume(Main.DEMO_DOMAIN, "Volume3",
                    new VolumePolicy(1024 * 4, VolumeConnector.S3));
            Thread.sleep(1000);
            xdi.createVolume(Main.DEMO_DOMAIN, "Volume4",
                    new VolumePolicy(1024 * 4, VolumeConnector.S3));
            Thread.sleep(1000);
        } catch (TException e) {
            //throw new RuntimeException(e);
        }
        return new String[] {"Volume1", "Volume2", "Volume3", "Volume4"};
    }
}
