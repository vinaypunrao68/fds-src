package com.formationds.web.toolkit;

import org.eclipse.jetty.http.HttpMethod;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.nio.NetworkTrafficSelectChannelConnector;

import java.util.function.Supplier;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


public class WebApp {
    private RouteFinder routeFinder;
    private String webDir;

    public WebApp(String webDir) {
        this.webDir = webDir;
        this.routeFinder = new RouteFinder();
    }

    public void route(HttpMethod method, String route, Supplier<RequestHandler> handler) {
        routeFinder.route(method, route, handler);
    }

    public void start(int httpPort) {
        Server server = new Server();
        ServerConnector connector = new NetworkTrafficSelectChannelConnector(server);
        connector.setPort(httpPort);
        connector.setHost("0.0.0.0");

        server.setConnectors(new Connector[]{
                connector,
        });

        Dispatcher dispatcher = new Dispatcher(routeFinder, webDir);
        server.setHandler(dispatcher);

        try {
            server.start();
            server.join();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
