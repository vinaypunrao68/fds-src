package com.formationds.web.toolkit;

import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.nio.NetworkTrafficSelectChannelConnector;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;

import java.util.function.Supplier;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class WebApp {
    private RouteFinder routeFinder;

    public WebApp() {
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

        ServletContextHandler contextHandler = new ServletContextHandler();
        contextHandler.setContextPath("/");
        server.setHandler(contextHandler);

        Dispatcher dispatcher = new Dispatcher(routeFinder);
        contextHandler.addServlet(new ServletHolder(dispatcher), "/");
        server.setHandler(contextHandler);

        try {
            server.start();
            server.join();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
