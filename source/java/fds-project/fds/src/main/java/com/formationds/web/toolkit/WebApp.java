/*
 * Copyright 2014-2016 Formation Data Systems, Inc.
 */
package com.formationds.web.toolkit;

import org.eclipse.jetty.server.HttpConnectionFactory;
import org.eclipse.jetty.server.SecureRequestCustomizer;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.SslConnectionFactory;
import org.eclipse.jetty.server.handler.HandlerCollection;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.util.thread.ExecutorThreadPool;
import org.eclipse.jetty.util.thread.ThreadPool;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ForkJoinPool;
import java.util.function.Supplier;

public class WebApp {

    private RouteFinder routeFinder = new RouteFinder();
    private String webDir;
    private List<AsyncRequestExecutor> executors = new ArrayList<>();

    private ExecutorService webServerExecutorService;

    /**
     * @param webDir to serve static resources from
     */
    public WebApp(String webDir) {
        this.webDir = webDir;
    }

    /**
     * Create a web app that does not server static resources.
     */
    public WebApp() {
        this(null);
    }

    /**
     *
     * @param webDir
     * @param exec
     */
    public WebApp(String webDir, ExecutorService exec) {
        this.webDir = webDir;
        this.webServerExecutorService = exec;
    }

    /**
     *
     * @param exec
     * @return this
     *
     * @throws IllegalStateException if there is already an executor configured and it has not been shutdown.
     */
    public WebApp setServerExecutorService(ExecutorService exec) {
        if (webServerExecutorService != null && !webServerExecutorService.isShutdown()) {
            throw new IllegalStateException("The current web server executor service is not shutdown.  " +
                    "Shut down the executor before overwriting it with a new one.");
        }
        this.webServerExecutorService = exec;
        return this;
    }

    /**
     * Add a route to a request handler
     *
     * @param method
     * @param route
     * @param handler
     *
     * @return this
     */
    public WebApp route(HttpMethod method, String route, Supplier<RequestHandler> handler) {
        routeFinder.route(method, route, handler);
        return this;
    }

    /**
     * Add an AsyncRequestExecutor that will try to execute each request.
     *
     * @param executor
     * @return this
     */
    // TODO: this is not actually used anywhere.  Dispatcher has code that
    // will call async executors if there are any, but no one ever sets them.
   public WebApp addAsyncExecutor(AsyncRequestExecutor executor) {
        executors.add(executor);
        return this;
    }

    public void start(HttpConfiguration httpConfiguration) {
        start(httpConfiguration, null);
    }

    public void start(HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration) {
        Server server = configureServer( httpConfiguration, httpsConfiguration );

        try {
            server.start();
            server.join();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * @return a new instance of the default executor service used if one is not explicitly set.  This
     * should only be called once and only if there is no executor configured.
     */
    protected ExecutorService createDefaultExecutorService() {
        return new ForkJoinPool(250);
    }

    /**
     *
     * @return the executor service
     */
    protected ExecutorService getExecutorService() {
        if ( webServerExecutorService == null ) {
            webServerExecutorService = createDefaultExecutorService();
        }
        return webServerExecutorService;
    }

    private ThreadPool configureThreadPool() {
        return new ExecutorThreadPool( getExecutorService() );
    }

    /**
     * @param httpConfiguration
     * @param httpsConfiguration
     *
     * @return the configured server
     */
    protected Server configureServer( HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration ) {
        ThreadPool tp = configureThreadPool();
        Server server = new Server(tp);

        ServerConnector connector = new ServerConnector(server);
        connector.setPort(httpConfiguration.getPort());
        connector.setHost(httpConfiguration.getHost());
        server.addConnector(connector);

        if (httpsConfiguration != null) {
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
            server.addConnector(sslConnector);
        }

        // Each handler in a handler collection is called regardless of whether or not
        // a particular handler completes the request.  This is in contrast to a ContextHandlerCollection,
        // which will stop trying additional handlers once one indicates it has handled the request.
        HandlerCollection handlers= new HandlerCollection(false);

        ServletContextHandler contextHandler = configureDispatcherContextHandler();
        handlers.addHandler(contextHandler);
        server.setHandler(handlers);
        return server;
    }

    /**
     * Configure the Dispatcher servlet context handler.  The default implementation
     * create the Dispatcher and adds it to a context handler with the context path of "/".
     *
     * This could be overridden in subclasses to, for example, add Servlet Filters or other
     * configuration as necessary.
     *
     * @param handlers
     */
    protected ServletContextHandler configureDispatcherContextHandler() {
        Dispatcher dispatcher = new Dispatcher(routeFinder, webDir, executors);
        ServletContextHandler contextHandler = new ServletContextHandler();
        contextHandler.setContextPath("/");
        contextHandler.addServlet(new ServletHolder(dispatcher), "/");
        return contextHandler;
    }
}
