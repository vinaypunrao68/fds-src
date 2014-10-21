package com.formationds.spike;


import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.config.ConnectionConfig;
import org.apache.http.impl.nio.DefaultHttpServerIODispatch;
import org.apache.http.impl.nio.DefaultNHttpServerConnection;
import org.apache.http.impl.nio.DefaultNHttpServerConnectionFactory;
import org.apache.http.impl.nio.reactor.DefaultListeningIOReactor;
import org.apache.http.impl.nio.reactor.IOReactorConfig;
import org.apache.http.nio.NHttpConnection;
import org.apache.http.nio.NHttpConnectionFactory;
import org.apache.http.nio.NHttpServerConnection;
import org.apache.http.nio.entity.NStringEntity;
import org.apache.http.nio.protocol.*;
import org.apache.http.nio.reactor.IOEventDispatch;
import org.apache.http.nio.reactor.ListeningIOReactor;
import org.apache.http.protocol.*;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.InetSocketAddress;

/**
 * HTTP/1.1 file server based on the non-blocking I/O model and capable of direct channel
 * (zero copy) data transfer.
 */
public class NHttpServer {

    public static void main(String[] args) throws Exception {
        int port = 8080;

        // Create HTTP protocol processing chain
        HttpProcessor httpproc = HttpProcessorBuilder.create()
                .add(new ResponseDate())
                .add(new ResponseServer("Test/1.1"))
                .add(new ResponseContent())
                .add(new ResponseConnControl()).build();
        // Create request handler registry
        UriHttpAsyncRequestHandlerMapper reqistry = new UriHttpAsyncRequestHandlerMapper();
        // Register the default handler for all URIs
        reqistry.register("*", new FancyHandler());
        // Create server-side HTTP protocol handler
        HttpAsyncService protocolHandler = new HttpAsyncService(httpproc, reqistry) {

            @Override
            public void connected(final NHttpServerConnection conn) {
                System.out.println(conn + ": connection open");
                super.connected(conn);
            }

            @Override
            public void closed(final NHttpServerConnection conn) {
                System.out.println(conn + ": connection closed");
                super.closed(conn);
            }

        };
        // Create HTTP connection factory
        NHttpConnectionFactory<DefaultNHttpServerConnection> connFactory;
        connFactory = new DefaultNHttpServerConnectionFactory(
                ConnectionConfig.DEFAULT);
        // Create server-side I/O event dispatch
        IOEventDispatch ioEventDispatch = new DefaultHttpServerIODispatch(protocolHandler, connFactory);
        // Set I/O reactor defaults
        IOReactorConfig config = IOReactorConfig.custom()
                .setIoThreadCount(1)
                .setSoTimeout(3000)
                .setConnectTimeout(3000)
                .build();
        // Create server-side I/O reactor
        ListeningIOReactor ioReactor = new DefaultListeningIOReactor(config);
        try {
            // Listen of the given port
            ioReactor.listen(new InetSocketAddress(port));
            // Ready to go!
            ioReactor.execute(ioEventDispatch);
        } catch (InterruptedIOException ex) {
            System.err.println("Interrupted");
        } catch (IOException e) {
            System.err.println("I/O error: " + e.getMessage());
        }
        System.out.println("Shutdown");
    }

    static class FancyHandler implements HttpAsyncRequestHandler<HttpRequest> {
        FancyHandler() {
        }

        public HttpAsyncRequestConsumer<HttpRequest> processRequest(
                final HttpRequest request,
                final HttpContext context) {
            // Buffer request content in memory for simplicity
            return new BasicAsyncRequestConsumer();
        }

        public void handle(
                final HttpRequest request,
                final HttpAsyncExchange httpexchange,
                final HttpContext context) throws HttpException, IOException {
            HttpResponse response = httpexchange.getResponse();
            HttpCoreContext coreContext = HttpCoreContext.adapt(context);
            NHttpConnection conn = coreContext.getConnection(NHttpConnection.class);
            response.setStatusCode(HttpStatus.SC_OK);
            response.setEntity(new NStringEntity("Hello, world!"));
            httpexchange.submitResponse(new BasicAsyncResponseProducer(response));
        }
    }
}