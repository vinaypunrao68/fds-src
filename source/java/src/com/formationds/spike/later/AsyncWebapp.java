package com.formationds.spike.later;

import com.formationds.spike.later.pathtemplate.RouteResult;
import com.formationds.spike.later.pathtemplate.RoutingMap;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.HttpsConfiguration;
import io.undertow.Undertow;
import io.undertow.io.IoCallback;
import io.undertow.io.Sender;
import io.undertow.server.HttpServerExchange;
import io.undertow.util.HeaderMap;
import io.undertow.util.HttpString;
import org.apache.log4j.Logger;
import org.eclipse.jetty.util.ssl.SslContextFactory;

import javax.net.ssl.SSLContext;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.ByteBuffer;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncWebapp {
    private static final Logger LOG = Logger.getLogger(AsyncWebapp.class);

    public static void _main(String[] args) throws Exception {
        AsyncWebapp app = new AsyncWebapp(new HttpConfiguration(8080), null);
        app.route(new HttpPath(HttpMethod.GET, "/"), ctx -> {
            return CompletableFuture.<Void>runAsync(() -> {
                Sender responseSender = ctx.getExchange().getResponseSender();
                responseSender.send("Foo\n");
                responseSender.send("bar\n");
                responseSender.close();
            });
        });

        app.start();
    }


    public static void main(final String[] args) {
        IoCallback cb = new IoCallback() {
            @Override
            public void onComplete(HttpServerExchange exchange, Sender sender) {

            }

            @Override
            public void onException(HttpServerExchange exchange, Sender sender, IOException exception) {

            }
        };

        Undertow server = Undertow.builder()
                .addHttpListener(8080, "localhost")
                .setHandler(exchange -> {
                    exchange.dispatch();
                    exchange.setPersistent(true);
                    CompletableFuture.runAsync(() -> {
                        try {
                            Thread.sleep(300);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        Sender sender = exchange.getResponseSender();
                        sender.send(ByteBuffer.wrap("Hello, world!\n".getBytes()), cb);
                        sender.send(ByteBuffer.wrap("Hello again!\n".getBytes()), cb);
                        sender.close();
                    });
                }).build();
        server.start();
    }

    private HttpConfiguration httpConfiguration;
    private HttpsConfiguration httpsConfiguration;
    private RoutingMap<Function<HttpContext, CompletableFuture<Void>>> routingMap;


    public AsyncWebapp(HttpConfiguration httpConfiguration, HttpsConfiguration httpsConfiguration) {
        this.httpConfiguration = httpConfiguration;
        this.httpsConfiguration = httpsConfiguration;
        routingMap = new RoutingMap<>();
    }

    public void route(HttpPath httpPath, Function<HttpContext, CompletableFuture<Void>> handler) {
        routingMap.put(httpPath, handler);
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
        PrintWriter pw = new PrintWriter(exchange.getOutputStream());
        ex.printStackTrace(pw);
        pw.close();
        return null;
    }

    protected void service(HttpServerExchange exchange) {
        RouteResult<Function<HttpContext, CompletableFuture<Void>>> matchResult = routingMap.get(exchange);

        if (matchResult.isRoutingSuccessful()) {
            LOG.debug(exchange.getRequestMethod() + " " + exchange.getRequestURI());
            Function<HttpContext, CompletableFuture<Void>> handler = matchResult.getResult();
            HttpContext rc = new HttpContext(exchange, matchResult.getRouteParameters());
            HeaderMap responseHeaders = exchange.getResponseHeaders();
            responseHeaders.add(new HttpString("Access-Control-Allow-Origin"), "*");
            responseHeaders.add(new HttpString("Server"), "Formation");

            exchange.dispatch();
            CompletableFuture<Void> cf = handler.apply(rc);

            cf.exceptionally(ex -> handleError(ex, exchange));
            cf.thenAccept(x -> {
                exchange.endExchange();
            });

            return;
        }

        exchange.setResponseCode(HttpServletResponse.SC_NOT_FOUND);
        exchange.getResponseHeaders().add(HttpString.tryFromString("Content-Type"), "text/plain");
        Sender sender = exchange.getResponseSender();
        sender.send("Four, oh, four! Resource not found");
        sender.close();
    }
}