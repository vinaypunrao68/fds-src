package com.formationds.spike.later;

import com.formationds.util.async.AsyncResourcePool;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.HttpsConfiguration;
import com.formationds.xdi.s3.VoidImpl;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.*;
import org.eclipse.jetty.server.handler.HandlerCollection;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.eclipse.jetty.util.MultiMap;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.util.thread.ExecutorThreadPool;
import org.eclipse.jetty.util.thread.ThreadPool;

import javax.servlet.AsyncContext;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ForkJoinPool;
import java.util.function.Function;

public class App extends HttpServlet {
    private static final Logger LOG = Logger.getLogger(App.class);

    public static void main(String[] args) throws Exception {
        App app = new App(new HttpConfiguration(8080), null);
        app.route(new HttpPath(HttpMethod.GET, "/"), ctx -> {
            CompletableFuture<Void> cf = null;
            try {
                cf = new CompletableFuture<>();
                ServletOutputStream out = ctx.getResponse().getOutputStream();
                out.write("Hello, world\n".getBytes());
                out.close();
                cf.complete(null);
            } catch (IOException e) {
                cf.completeExceptionally(e);
            }
            return cf;
        });

        app.start();
    }

    private HttpConfiguration httpConfiguration;
    private HttpsConfiguration httpsConfiguration;
    private final AsyncResourcePool<Void> requestLimiter;
    private List<HttpPath> paths;
    private List<Function<HttpPathContext, CompletableFuture<Void>>> handlers;

    public static final int CONCURRENCY = 500;

    public App(HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration) {
        this.httpConfiguration = httpConfiguration;
        this.httpsConfiguration = httpsConfiguration;
        paths = new ArrayList<>();
        handlers = new ArrayList<>();
        this.requestLimiter = new AsyncResourcePool<>(new VoidImpl(), CONCURRENCY);
    }

    public void route(HttpPath httpPath, Function<HttpPathContext, CompletableFuture<Void>> handler) {
        paths.add(httpPath);
        handlers.add(handler);
    }

    public void start() {
        System.setProperty("org.eclipse.jetty.server.Request.maxFormContentSize", "100000000");
        ThreadPool tp = new ExecutorThreadPool(new ForkJoinPool(250));
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

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doHead(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
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
    protected void doOptions(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doTrace(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    private Void handleError(Throwable ex, Response resp) {
        try {
            LOG.debug("Error: " + ex);
            resp.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
            resp.setContentType("text/plain");
            ServletOutputStream out = resp.getOutputStream();
            PrintWriter pw = new PrintWriter(out);
            ex.printStackTrace(pw);
            pw.flush();
            pw.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return null;
    }

    @Override
    protected void service(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        Request request = (Request) req;
        Response response = (Response) resp;

        if (request.getQueryParameters() == null) {
            MultiMap<String> qp = new MultiMap<>();
            request.getUri().decodeQueryTo(qp);
            request.setQueryParameters(qp);
        }

        for (int i = 0; i < paths.size(); i++) {
            HttpPath httpPath = paths.get(i);

            if (httpPath.matches(request)) {
                LOG.debug(request.getMethod() + " " + request.getRequestURI());
                Function<HttpPathContext, CompletableFuture<Void>> handler = handlers.get(i);
                HttpPathContext rc = new HttpPathContext(request, response, httpPath.getRouteParameters());
                AsyncContext asyncContext = request.startAsync();
                asyncContext.setTimeout(Long.MAX_VALUE);

                response.addHeader("Access-Control-Allow-Origin", "*");
                response.setHeader("Server", "Formation");

                CompletableFuture<Void> cf = handler.apply(rc);

                requestLimiter.use(_null ->
                        cf.exceptionally(ex -> handleError(ex, response))
                                .whenComplete((x, ex) -> {
                                    asyncContext.complete();
                                }));

                return;
            }
        }

        resp.setStatus(HttpServletResponse.SC_NOT_FOUND);
        resp.setContentType("text/plain");
        ServletOutputStream out = resp.getOutputStream();
        out.write("Four, oh, four! Resource not found".getBytes());
        out.close();
    }
}