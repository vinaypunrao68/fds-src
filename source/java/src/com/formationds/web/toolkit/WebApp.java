package com.formationds.web.toolkit;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;

import java.util.function.Supplier;

import java.util.concurrent.ForkJoinPool;
import org.eclipse.jetty.util.thread.ExecutorThreadPool;
import org.eclipse.jetty.util.thread.ThreadPool;

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

    public WebApp() {
        this.webDir = null;
        this.routeFinder = new RouteFinder();
    }

    public void route(HttpMethod method, String route, Supplier<RequestHandler> handler) {
        routeFinder.route(method, route, handler);
    }

    public void start(int httpPort) {
        ThreadPool tp = new ExecutorThreadPool(new ForkJoinPool(250));
        Server server = new Server(tp);
        ServerConnector connector = new ServerConnector(server);
        connector.setHost("0.0.0.0");
        connector.setPort(httpPort);

        server.addConnector(connector);

        ServletContextHandler contextHandler = new ServletContextHandler();
        contextHandler.setContextPath("/");
        server.setHandler(contextHandler);

        Dispatcher dispatcher = new Dispatcher(routeFinder, webDir);
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
