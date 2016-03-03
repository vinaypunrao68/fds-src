package com.formationds.spike.later;

import com.formationds.spike.later.pathtemplate.RouteResult;
import com.formationds.spike.later.pathtemplate.RoutingMap;
import com.formationds.util.async.AsyncSemaphore;
import com.formationds.web.toolkit.FourOhFour;
import com.formationds.web.toolkit.HttpsConfiguration;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.*;
import org.eclipse.jetty.server.handler.HandlerCollection;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.util.thread.ExecutorThreadPool;
import org.eclipse.jetty.util.thread.ThreadPool;

import javax.servlet.AsyncContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ForkJoinPool;
import java.util.function.Function;

public class AsyncWebapp extends HttpServlet {
    private static final Logger LOG = Logger.getLogger(AsyncWebapp.class);

    private com.formationds.web.toolkit.HttpConfiguration httpConfiguration;
    private HttpsConfiguration httpsConfiguration;
    private RoutingMap<Function<HttpContext, CompletableFuture<Void>>> routingMap;
    private Server server;
    private AsyncSemaphore semaphore;


    public AsyncWebapp(com.formationds.web.toolkit.HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration) {
        this.httpConfiguration = httpConfiguration;
        this.httpsConfiguration = httpsConfiguration;
        routingMap = new RoutingMap<>();
    }

    public AsyncWebapp(com.formationds.web.toolkit.HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration, int maxConcurrentRequests) {
        this(httpConfiguration, httpsConfiguration);
        if(maxConcurrentRequests < 1)
            throw new IllegalArgumentException("maxConcurrentRequests is cannot be less than zero");
        semaphore = new AsyncSemaphore(maxConcurrentRequests);
    }

    public void route(HttpPath httpPath, Function<HttpContext, CompletableFuture<Void>> handler) {
        routingMap.put(httpPath, handler);
    }

    public void stop() {
        try {
            server.stop();
            server.join();
            server = null;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public void start() {
        ThreadPool tp = new ExecutorThreadPool(new ForkJoinPool(250));
        server = new Server(tp);

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
        HandlerCollection handlers = new HandlerCollection(false);

        ServletContextHandler contextHandler = new ServletContextHandler();
        contextHandler.setContextPath("/");
        contextHandler.addServlet(new ServletHolder(this), "/");
        handlers.addHandler(contextHandler);

        server.setHandler(handlers);

        try {
            server.start();
            server.join();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private Void handleError(Throwable ex, HttpContext ctx) {
        LOG.debug("Error executing handler for " + ctx.getRequestMethod() + " " + ctx.getRequestURI(), ex);
        ctx.setResponseStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        ctx.setResponseContentType("text/plain");
        PrintWriter pw = new PrintWriter(ctx.getOutputStream());
        ex.printStackTrace(pw);
        pw.close();
        return null;
    }


    @Override
    protected void service(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        Request request = (Request) req;
        Response response = (Response) resp;
        HttpContext context = new HttpContext(request, response);
        RouteResult<Function<HttpContext, CompletableFuture<Void>>> matchResult = routingMap.get(context);

        if (matchResult.isRoutingSuccessful()) {
            AsyncContext asyncContext = request.startAsync();
            asyncContext.setTimeout(1000 * 60 * 30);
            LOG.debug(req.getMethod() + " " + req.getRequestURI());
            Function<HttpContext, CompletableFuture<Void>> handler = matchResult.getResult();
            response.addHeader("Access-Control-Allow-Origin", "*");
            response.addHeader("Server", "Formation");
            CompletableFuture<Void> requestHandledFuture;
            if(semaphore != null)
                requestHandledFuture = semaphore.execute(() -> handler.apply(context.withRouteParameters(matchResult.getRouteParameters())));
            else
                requestHandledFuture = handler.apply(context.withRouteParameters(matchResult.getRouteParameters()));

            requestHandledFuture.exceptionally(ex -> handleError(ex, context));
            requestHandledFuture.whenComplete((_x, _z) -> asyncContext.complete());
            return;
        }

        new FourOhFour().renderTo(new HttpContext(request, response, new HashMap<String, String>()));
    }

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doPut(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doDelete(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doHead(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

}