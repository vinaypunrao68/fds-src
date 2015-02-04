
package com.formationds.spike;

import org.vertx.java.core.Handler;
import org.vertx.java.core.Vertx;
import org.vertx.java.core.VertxFactory;
import org.vertx.java.core.buffer.Buffer;
import org.vertx.java.core.http.HttpServerRequest;
import org.vertx.java.core.http.HttpServerResponse;
import org.vertx.java.platform.Verticle;

import java.util.Random;

public class VertX extends Verticle {
    private static final byte[] BUF = new byte[4096];

    static {
        new Random().nextBytes(BUF);
    }

    public static void main(String[] args) {
        new VertX().start();
    }
    public void start() {
        Vertx vertx = VertxFactory.newVertx();
        vertx.createHttpServer().requestHandler(new Handler<HttpServerRequest>() {
            public void handle(HttpServerRequest req) {
                HttpServerResponse response = req.response();
                response.end(new Buffer(BUF));
                response.setWriteQueueMaxSize(Integer.MAX_VALUE);
            }
        }).listen(8000);
    }
}