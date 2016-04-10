package com.formationds.nfs;

import com.formationds.web.toolkit.*;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class ControlServer {
    private static final Logger LOG = LogManager.getLogger(ControlServer.class);
    private IoOps ops;
    private final HttpConfiguration configuration;
    private final WebApp webApp;

    public ControlServer(int httpPort, IoOps ops) {
        this.ops = ops;
        webApp = new WebApp();
        webApp.route(HttpMethod.GET, "/flush/:volume", () -> new FlushVolume());
        configuration = new HttpConfiguration(httpPort, "localhost");
    }

    public void start() {
        Thread thread = new Thread(() -> {
            webApp.start(configuration);
        });
        thread.setName("XDI control server, port=" + configuration.getPort());
        thread.start();
    }

    private class FlushVolume implements RequestHandler {
        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            String volumeName = requiredString(routeParameters, "volume");
            try {
                ops.commitAll(XdiVfs.DOMAIN, volumeName);
                return new TextResource(200, "OK");
            } catch (Exception e) {
                return new ErrorPage("Error flushing volume " + volumeName, e);
            }

        }
    }

    ;
}
