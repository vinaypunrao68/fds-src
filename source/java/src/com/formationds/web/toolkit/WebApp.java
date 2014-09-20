package com.formationds.web.toolkit;

import org.eclipse.jetty.server.*;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.util.thread.ExecutorThreadPool;
import org.eclipse.jetty.util.thread.ThreadPool;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ForkJoinPool;
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
    private List<AsyncRequestExecutor> executors;

    public WebApp(String webDir) {
        this.webDir = webDir;
        this.routeFinder = new RouteFinder();
    }

    public WebApp() {
        this.webDir = null;
        this.routeFinder = new RouteFinder();
        this.executors = new ArrayList<>();
    }

    public void route(HttpMethod method, String route, Supplier<RequestHandler> handler) {
        routeFinder.route(method, route, handler);
    }

    public void addAsyncExecutor(AsyncRequestExecutor executor) {
        executors.add(executor);
    }

    public void start(HttpConfiguration httpConfiguration) {
        start(httpConfiguration, null);
    }

    public void _start(HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration) {
        Server server = new Server();

        ServerConnector connector = new ServerConnector(server);
        connector.setPort(7779);

        org.eclipse.jetty.server.HttpConfiguration https = new org.eclipse.jetty.server.HttpConfiguration();
        https.addCustomizer(new SecureRequestCustomizer());

        SslContextFactory sslContextFactory = new SslContextFactory();
        sslContextFactory.setKeyStorePath("/home/fab/FDS/dev/source/config/etc/ssl/dev/keystore");
        sslContextFactory.setKeyStorePassword("exotic sparrow");
        sslContextFactory.setKeyManagerPassword("exotic sparrow");

        ServerConnector sslConnector = new ServerConnector(server,
                new SslConnectionFactory(sslContextFactory, "http/1.1"),
                new HttpConnectionFactory(https));
        sslConnector.setPort(7443);

        server.setConnectors(new Connector[] { connector, sslConnector });
        try {
            server.start();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void start(HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration) {
        ThreadPool tp = new ExecutorThreadPool(new ForkJoinPool(250));
        Server server = new Server(tp);

        ServerConnector connector = new ServerConnector(server);
        connector.setPort(httpConfiguration.getPort());
        connector.setHost(httpConfiguration.getHost());

        org.eclipse.jetty.server.HttpConfiguration https = new org.eclipse.jetty.server.HttpConfiguration();
        https.addCustomizer(new SecureRequestCustomizer());

        SslContextFactory sslContextFactory = new SslContextFactory();
        sslContextFactory.setKeyStorePath(httpsConfiguration.getKeyStore().getAbsolutePath());
        sslContextFactory.setKeyStorePassword(httpsConfiguration.getKeystorePassword());
        sslContextFactory.setKeyManagerPassword(httpsConfiguration.getKeystorePassword());

        ServerConnector sslConnector = new ServerConnector(server,
                new SslConnectionFactory(sslContextFactory, "http/1.1"),
                new HttpConnectionFactory(https));
        sslConnector.setPort(httpsConfiguration.getPort());
        sslConnector.setHost(httpsConfiguration.getHost());

        server.setConnectors(new Connector[] { connector, sslConnector });

        ServletContextHandler contextHandler = new ServletContextHandler();
        contextHandler.setContextPath("/");
        server.setHandler(contextHandler);

        Dispatcher dispatcher = new Dispatcher(routeFinder, webDir, executors);
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
