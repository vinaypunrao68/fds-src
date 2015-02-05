package com.formationds.spike;

import io.undertow.Undertow;
import io.undertow.io.IoCallback;
import io.undertow.io.Sender;
import io.undertow.server.HttpHandler;
import io.undertow.server.HttpServerExchange;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Random;

public class UndertowTest {
    public static void main(String[] args) {
        byte[] bytes = new byte[4096];
        new Random().nextBytes(bytes);

        IoCallback callback = new IoCallback() {
            @Override
            public void onComplete(HttpServerExchange exchange, Sender sender) {
            }

            @Override
            public void onException(HttpServerExchange exchange, Sender sender, IOException exception) {
            }
        };

        Undertow server = Undertow.builder()
                .addHttpListener(8000, "localhost")
                .setHandler(new HttpHandler() {
                    @Override
                    public void handleRequest(final HttpServerExchange exchange) throws Exception {
                        Sender responseSender = exchange.getResponseSender();
                        responseSender.send(ByteBuffer.wrap(bytes), callback);
                        responseSender.close(callback);
                    }
                }).build();
        server.start();
    }
}
