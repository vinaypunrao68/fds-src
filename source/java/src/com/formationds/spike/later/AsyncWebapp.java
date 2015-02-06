package com.formationds.spike.later;

import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.HttpsConfiguration;
import io.undertow.Undertow;
import io.undertow.io.Sender;
import io.undertow.server.HttpServerExchange;
import io.undertow.util.HeaderMap;
import io.undertow.util.HttpString;
import org.apache.log4j.Logger;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.xnio.channels.StreamSinkChannel;

import javax.net.ssl.SSLContext;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncWebapp {
    private static final Logger LOG = Logger.getLogger(AsyncWebapp.class);

    public static void main(String[] args) throws Exception {
        AsyncWebapp app = new AsyncWebapp(new HttpConfiguration(8080), null);
        app.route(new HttpPath(HttpMethod.GET, "/"), ctx -> {
            ctx.addResponseHeader("Foo", "Bar");
            PrintWriter pw = new PrintWriter(ctx.getOutputStream());
            pw.println("Hello, world");
            pw.close();
            return CompletableFuture.completedFuture(null);
        });

        app.start();
    }

    private HttpConfiguration httpConfiguration;
    private HttpsConfiguration httpsConfiguration;
    private List<HttpPath> paths;
    private List<Function<HttpContext, CompletableFuture<Void>>> handlers;

    public AsyncWebapp(HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration) {
        this.httpConfiguration = httpConfiguration;
        this.httpsConfiguration = httpsConfiguration;
        paths = new ArrayList<>();
        handlers = new ArrayList<>();
    }

    public void route(HttpPath httpPath, Function<HttpContext, CompletableFuture<Void>> handler) {
        paths.add(httpPath);
        handlers.add(handler);
    }

    public void start() {
        Undertow.Builder builder = Undertow.builder()
                .addHttpListener(httpConfiguration.getPort(), "0.0.0.0")
                .setHandler(exchange -> service(exchange));

        if (httpsConfiguration != null) {
            SslContextFactory sslContextFactory = new SslContextFactory();
            sslContextFactory.setKeyStorePath(httpsConfiguration.getKeyStore().getAbsolutePath());
            sslContextFactory.setKeyStorePassword(httpsConfiguration.getKeystorePassword());
            sslContextFactory.setKeyManagerPassword(httpsConfiguration.getKeystorePassword());
            try {
                sslContextFactory.start();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            SSLContext sslContext = sslContextFactory.getSslContext();
            builder.addHttpsListener(httpsConfiguration.getPort(), "0.0.0.0", sslContext);
        }

        Undertow undertow = builder.build();
        undertow.start();

    }

    private Void handleError(Throwable ex, HttpServerExchange exchange) {
        LOG.debug("Error executing handler for " + exchange.getRequestMethod() + " " + exchange.getRequestURI(), ex);
        exchange.setResponseCode(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        exchange.getResponseHeaders().add(HttpString.tryFromString("Content-Type"), "text/plain");
        exchange.startBlocking();
        PrintWriter pw = new PrintWriter(exchange.getOutputStream());
        ex.printStackTrace(pw);
        pw.close();
        return null;
    }

    protected void service(HttpServerExchange exchange) {
        for (int i = 0; i < paths.size(); i++) {
            HttpPath httpPath = paths.get(i);

            if (httpPath.matches(exchange)) {
                LOG.debug(exchange.getRequestMethod() + " " + exchange.getRequestURI());
                Function<HttpContext, CompletableFuture<Void>> handler = handlers.get(i);
                HttpContext rc = new HttpContext(exchange, httpPath.getRouteParameters());
                HeaderMap responseHeaders = exchange.getResponseHeaders();
                responseHeaders.add(new HttpString("Access-Control-Allow-Origin"), "*");
                responseHeaders.add(new HttpString("Server"), "Formation");

                CompletableFuture<Void> cf = handler.apply(rc);

                cf.exceptionally(ex -> handleError(ex, exchange));
                cf.thenAccept(x -> {
                    try {
                        StreamSinkChannel responseChannel = exchange.getResponseChannel();
                        responseChannel.flush();
                        responseChannel.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                });
                return;
            }
        }

        exchange.setResponseCode(HttpServletResponse.SC_NOT_FOUND);
        exchange.getResponseHeaders().add(HttpString.tryFromString("Content-Type"), "text/plain");
        Sender sender = exchange.getResponseSender();
        sender.send("Four, oh, four! Resource not found");
        sender.close();
    }
}